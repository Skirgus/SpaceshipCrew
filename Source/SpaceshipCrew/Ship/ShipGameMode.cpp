// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShipGameMode.h"
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

void AShipGameMode::EnsureCrewSpawned()
{
	if (bCrewSpawned)
	{
		return;
	}
	if (!CrewPawnClass)
	{
		return;
	}
	EnsureDefaultRoles();
	if (MandatoryRoles.Num() == 0)
	{
		return;
	}

	PlayerRoleSlotIndex = FMath::Clamp(PlayerRoleSlotIndex, 0, MandatoryRoles.Num() - 1);

	AShipActor* Ship = FindShipActor();
	FVector Fallback = FVector::ZeroVector;
	if (AActor* Start = UGameplayStatics::GetActorOfClass(GetWorld(), APlayerStart::StaticClass()))
	{
		Fallback = Start->GetActorLocation();
	}

	CrewPawns.SetNum(MandatoryRoles.Num());

	for (int32 i = 0; i < MandatoryRoles.Num(); ++i)
	{
		const FTransform Xform = GetSpawnTransformForSlot(i, Ship, Fallback);
		AShipCrewCharacter* Spawned = GetWorld()->SpawnActor<AShipCrewCharacter>(CrewPawnClass, Xform);
		if (!Spawned)
		{
			continue;
		}
		CrewPawns[i] = Spawned;
		if (UCrewRoleComponent* RoleComp = Spawned->CrewRole)
		{
			RoleComp->SetRoleDefinition(MandatoryRoles[i]);
		}

		const bool bIsHumanSlot = (i == PlayerRoleSlotIndex);
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
			ACrewAIController* AI = GetWorld()->SpawnActor<ACrewAIController>(ACrewAIController::StaticClass(), Params);
			if (AI)
			{
				AI->Possess(Spawned);
			}
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

APawn* AShipGameMode::SpawnDefaultPawnFor(AController* NewPlayer, AActor* StartSpot)
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

