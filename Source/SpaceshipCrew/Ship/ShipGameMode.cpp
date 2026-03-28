// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShipGameMode.h"
#include "SpaceshipCrew.h"
#include "ShipPlayerController.h"
#include "CrewAIController.h"
#include "CrewBotBrainComponent.h"
#include "CrewRoleComponent.h"
#include "CrewRoleDefinition.h"
#include "ShipActor.h"
#include "ShipCrewCharacter.h"
#include "ShipCrewManifest.h"
#include "ShipGameState.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

AShipGameMode::AShipGameMode()
{
	GameStateClass = AShipGameState::StaticClass();
	PlayerControllerClass = AShipPlayerController::StaticClass();
	DefaultPawnClass = AShipCrewCharacter::StaticClass();
	CrewPawnClass = AShipCrewCharacter::StaticClass();
}

void AShipGameMode::BeginPlay()
{
	Super::BeginPlay();
	EnsureDefaultRoles();
	EnsureCrewSpawned();
}

void AShipGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	RebuildGameStateCrewSlots();
}

void AShipGameMode::EnsureDefaultRoles()
{
	if (CrewManifest && CrewManifest->MandatoryRoles.Num() > 0)
	{
		MandatoryRoles = CrewManifest->MandatoryRoles;
	}
	if (MandatoryRoles.Num() > 0)
	{
		return;
	}

	auto MakeRole = [this](FName Id, const TArray<FName>& Perms) -> UCrewRoleDefinition*
	{
		UCrewRoleDefinition* R = NewObject<UCrewRoleDefinition>(this);
		R->RoleId = Id;
		R->DisplayName = FText::FromName(Id);
		for (FName P : Perms)
		{
			R->AllowedStationPermissions.Add(P);
		}
		return R;
	};

	MandatoryRoles.Add(MakeRole(FName(TEXT("Captain")), TArray<FName>{FName(TEXT("Helm"))}));
	MandatoryRoles.Add(MakeRole(FName(TEXT("Engineer")), TArray<FName>{FName(TEXT("Reactor")), FName(TEXT("Extinguisher"))}));
	MandatoryRoles.Add(MakeRole(FName(TEXT("Medic")), TArray<FName>{FName(TEXT("Medical"))}));
	MandatoryRoles.Add(MakeRole(FName(TEXT("Pilot")), TArray<FName>{FName(TEXT("Helm"))}));
}

AShipActor* AShipGameMode::FindShipActor() const
{
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AShipActor> It(World); It; ++It)
		{
			return *It;
		}
	}
	return nullptr;
}

FTransform AShipGameMode::GetSpawnTransformForSlot(int32 SlotIndex, AShipActor* Ship, const FVector& FallbackLocation) const
{
	if (Ship && Ship->CrewSpawnTransforms.IsValidIndex(SlotIndex))
	{
		return Ship->CrewSpawnTransforms[SlotIndex] * Ship->GetActorTransform();
	}
	if (Ship)
	{
		const FVector Offset(float(SlotIndex) * 150.f, 0.f, 92.f);
		return FTransform(Ship->GetActorRotation(), Ship->GetActorLocation() + Offset);
	}
	return FTransform(FRotator::ZeroRotator, FallbackLocation);
}

void AShipGameMode::FinalizeCrewPawnSpawn(AShipCrewCharacter* Spawned, int32 SlotIndex)
{
	if (!Spawned)
	{
		return;
	}
	if (UCrewRoleComponent* RoleComp = Spawned->CrewRole)
	{
		if (MandatoryRoles.IsValidIndex(SlotIndex) && MandatoryRoles[SlotIndex])
		{
			RoleComp->SetRoleDefinition(MandatoryRoles[SlotIndex]);
		}
	}

	const bool bIsHumanSlot = (SlotIndex == PlayerRoleSlotIndex);
	if (bIsHumanSlot)
	{
		if (Spawned->CrewBotBrain)
		{
			Spawned->CrewBotBrain->bBrainEnabled = false;
		}
	}
	else
	{
		if (Spawned->CrewBotBrain)
		{
			Spawned->CrewBotBrain->bBrainEnabled = true;
		}
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (ACrewAIController* AI = GetWorld()->SpawnActor<ACrewAIController>(ACrewAIController::StaticClass(), Params))
		{
			AI->Possess(Spawned);
		}
	}
}

void AShipGameMode::EnsureCrewSpawned()
{
	if (bCrewSpawned)
	{
		return;
	}

	if (!CrewPawnClass)
	{
		if (DefaultPawnClass && DefaultPawnClass->IsChildOf(AShipCrewCharacter::StaticClass()))
		{
			CrewPawnClass = TSubclassOf<AShipCrewCharacter>(DefaultPawnClass);
		}
		else
		{
			CrewPawnClass = AShipCrewCharacter::StaticClass();
		}
	}

	if (!CrewPawnClass)
	{
		UE_LOG(LogSpaceshipCrew, Error, TEXT("ShipGameMode: CrewPawnClass не задан и DefaultPawnClass не ShipCrewCharacter."));
		return;
	}

	EnsureDefaultRoles();
	if (MandatoryRoles.Num() == 0)
	{
		return;
	}

	PlayerRoleSlotIndex = FMath::Clamp(PlayerRoleSlotIndex, 0, MandatoryRoles.Num() - 1);

	AShipActor* Ship = FindShipActor();
	FVector Fallback(0.f, 0.f, 300.f);
	if (AActor* Start = UGameplayStatics::GetActorOfClass(GetWorld(), APlayerStart::StaticClass()))
	{
		Fallback = Start->GetActorLocation();
	}

	CrewPawns.SetNum(MandatoryRoles.Num());
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	for (int32 i = 0; i < MandatoryRoles.Num(); ++i)
	{
		const FTransform Xform = GetSpawnTransformForSlot(i, Ship, Fallback);
		AShipCrewCharacter* Spawned = GetWorld()->SpawnActor<AShipCrewCharacter>(CrewPawnClass, Xform, SpawnParams);
		if (!Spawned)
		{
			UE_LOG(LogSpaceshipCrew, Warning, TEXT("ShipGameMode: SpawnActor вернул null для слота %d (класс %s)."),
				i, *GetNameSafe(CrewPawnClass.Get()));
			continue;
		}
		CrewPawns[i] = Spawned;
		FinalizeCrewPawnSpawn(Spawned, i);
	}

	if (!CrewPawns.IsValidIndex(PlayerRoleSlotIndex) || CrewPawns[PlayerRoleSlotIndex] == nullptr)
	{
		const FVector HumanLoc = Fallback + FVector(0.f, 0.f, 92.f);
		const FTransform HumanXform(FRotator::ZeroRotator, HumanLoc);
		if (AShipCrewCharacter* Spawned = GetWorld()->SpawnActor<AShipCrewCharacter>(CrewPawnClass, HumanXform, SpawnParams))
		{
			CrewPawns[PlayerRoleSlotIndex] = Spawned;
			FinalizeCrewPawnSpawn(Spawned, PlayerRoleSlotIndex);
			UE_LOG(LogSpaceshipCrew, Warning, TEXT("ShipGameMode: слот игрока %d заспавнен у PlayerStart (резерв): проверьте CrewSpawnTransforms и Z над палубой."),
				PlayerRoleSlotIndex);
		}
		else
		{
			UE_LOG(LogSpaceshipCrew, Error, TEXT("ShipGameMode: не удалось заспавнить игрока даже в резервной точке."));
		}
	}

	bCrewSpawned = true;
	RebuildGameStateCrewSlots();
}

void AShipGameMode::RebuildGameStateCrewSlots()
{
	AShipGameState* S = GetGameState<AShipGameState>();
	if (!S)
	{
		return;
	}
	TArray<FCrewSlotReplicationData> Slots;
	Slots.Reserve(CrewPawns.Num());
	for (int32 i = 0; i < CrewPawns.Num(); ++i)
	{
		FCrewSlotReplicationData D;
		D.SlotIndex = i;
		D.RoleId = MandatoryRoles.IsValidIndex(i) && MandatoryRoles[i] ? MandatoryRoles[i]->RoleId : NAME_None;
		D.ControllerKind = (i == PlayerRoleSlotIndex) ? ECrewControllerKind::Human : ECrewControllerKind::Bot;
		D.AssignedPawn = CrewPawns[i];
		Slots.Add(D);
	}
	S->SetCrewSlotsFromAuthority(Slots);
}

APawn* AShipGameMode::SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot)
{
	EnsureCrewSpawned();
	if (APlayerController* PC = Cast<APlayerController>(NewPlayer))
	{
		if (CrewPawns.IsValidIndex(PlayerRoleSlotIndex) && CrewPawns[PlayerRoleSlotIndex])
		{
			return CrewPawns[PlayerRoleSlotIndex];
		}
	}
	return nullptr;
}

void AShipGameMode::PossessCrewSlot(APlayerController* Player, int32 SlotIndex)
{
	// В будущем: переносить игрока в SlotIndex и снимать управление с бота в этом слоте.
}

