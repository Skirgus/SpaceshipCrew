// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShipGameMode.h"
#include "SpaceshipCrew.h"
#include "ShipPlayerController.h"
#include "CrewAIController.h"
#include "CrewBotBrainComponent.h"
#include "CrewRoleComponent.h"
#include "CrewRoleDefinition.h"
#include "ShipConfigAsset.h"
#include "ShipActor.h"
#include "CrewSpawnMarkerComponent.h"
#include "ShipCrewCharacter.h"
#include "ShipCrewManifest.h"
#include "ShipGameState.h"
#include "EngineUtils.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

namespace
{
bool IsFiniteVector(const FVector& V)
{
	return FMath::IsFinite(V.X) && FMath::IsFinite(V.Y) && FMath::IsFinite(V.Z);
}

bool IsValidWorldSpawnTransform(const FTransform& W)
{
	const FVector L = W.GetLocation();
	return IsFiniteVector(L) && L.SizeSquared() < 1.e12f;
}

bool TryConsumeMarkerTransform(UCrewSpawnMarkerComponent* M, const TSet<UCrewSpawnMarkerComponent*>& UsedMarkers, FTransform& OutWorld)
{
	if (!M || UsedMarkers.Contains(M))
	{
		return false;
	}
	const FTransform W = M->GetComponentTransform();
	if (!IsValidWorldSpawnTransform(W))
	{
		UE_LOG(LogSpaceshipCrew, Warning, TEXT("ShipGameMode: CrewSpawnMarker %s даёт некорректный мир — пропуск."), *GetNameSafe(M));
		return false;
	}
	OutWorld = W;
	return true;
}

void BuildCrewSpawnTransformsInternal(
	const TArray<TObjectPtr<UCrewRoleDefinition>>& MandatoryRoles,
	AShipActor* Ship,
	const FVector& FallbackLocation,
	TArray<FTransform>& OutPerSlot)
{
	const int32 N = MandatoryRoles.Num();
	OutPerSlot.SetNum(N);
	TArray<bool> Assigned;
	Assigned.Init(false, N);

	if (N == 0)
	{
		return;
	}

	auto ApplyDefaultForSlot = [&](int32 SlotIndex)
	{
		if (Ship)
		{
			FVector BoundsOrigin = Ship->GetActorLocation();
			FVector BoundsExtent(400.f, 400.f, 120.f);
			Ship->GetActorBounds(true, BoundsOrigin, BoundsExtent);
			const FVector RandomOffset(
				FMath::FRandRange(-BoundsExtent.X * 0.8f, BoundsExtent.X * 0.8f),
				FMath::FRandRange(-BoundsExtent.Y * 0.8f, BoundsExtent.Y * 0.8f),
				FMath::Max(92.f, BoundsExtent.Z * 0.25f)
			);
			OutPerSlot[SlotIndex] = FTransform(Ship->GetActorRotation(), BoundsOrigin + RandomOffset);
		}
		else
		{
			OutPerSlot[SlotIndex] = FTransform(FRotator::ZeroRotator, FallbackLocation + FVector(float(SlotIndex) * 150.f, 0.f, 92.f));
		}
		Assigned[SlotIndex] = true;
	};

	if (!Ship)
	{
		for (int32 i = 0; i < N; ++i)
		{
			ApplyDefaultForSlot(i);
		}
		return;
	}

	TArray<UCrewSpawnMarkerComponent*> Markers;
	Ship->GetComponents<UCrewSpawnMarkerComponent>(Markers);
	TSet<UCrewSpawnMarkerComponent*> UsedMarkers;

	// 1) Маркеры с заданным RoleId → слот с той же ролью (первый подходящий по порядку компонентов).
	for (int32 i = 0; i < N; ++i)
	{
		const UCrewRoleDefinition* Def = MandatoryRoles[i].Get();
		if (!Def || Def->RoleId.IsNone())
		{
			continue;
		}
		const FName Wanted = Def->RoleId;
		for (UCrewSpawnMarkerComponent* M : Markers)
		{
			if (!M || M->RoleId.IsNone() || M->RoleId != Wanted)
			{
				continue;
			}
			FTransform W;
			if (TryConsumeMarkerTransform(M, UsedMarkers, W))
			{
				OutPerSlot[i] = W;
				UsedMarkers.Add(M);
				Assigned[i] = true;
				break;
			}
		}
	}

	// 2) Маркеры без RoleId → любой ещё не назначенный слот (по возрастанию индекса слота).
	TArray<UCrewSpawnMarkerComponent*> Wildcards;
	for (UCrewSpawnMarkerComponent* M : Markers)
	{
		if (M && M->RoleId.IsNone() && !UsedMarkers.Contains(M))
		{
			Wildcards.Add(M);
		}
	}
	int32 WIdx = 0;
	for (int32 i = 0; i < N; ++i)
	{
		if (Assigned[i])
		{
			continue;
		}
		FTransform Tm;
		while (WIdx < Wildcards.Num())
		{
			UCrewSpawnMarkerComponent* M = Wildcards[WIdx++];
			if (TryConsumeMarkerTransform(M, UsedMarkers, Tm))
			{
				OutPerSlot[i] = Tm;
				UsedMarkers.Add(M);
				Assigned[i] = true;
				break;
			}
		}
	}

	// 3) Остальные — сетка по умолчанию.
	for (int32 i = 0; i < N; ++i)
	{
		if (!Assigned[i])
		{
			ApplyDefaultForSlot(i);
		}
	}
}
} // namespace

AShipGameMode::AShipGameMode()
{
	GameStateClass = AShipGameState::StaticClass();
	PlayerControllerClass = AShipPlayerController::StaticClass();
	DefaultPawnClass = AShipCrewCharacter::StaticClass();
	CrewPawnClass = AShipCrewCharacter::StaticClass();
	ShipActorClass = AShipActor::StaticClass();
}

void AShipGameMode::BeginPlay()
{
	Super::BeginPlay();
	EnsureShipSpawnedAndConfigured();
	EnsureDefaultRoles();
	EnsureCrewSpawned();
}

void AShipGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	RebuildGameStateCrewSlots();
}

void AShipGameMode::RestartPlayer(AController* NewPlayer)
{
	Super::RestartPlayer(NewPlayer);

	APlayerController* PC = Cast<APlayerController>(NewPlayer);
	if (!PC)
	{
		return;
	}

	if (AShipCrewCharacter* Human = SpawnOrRetrieveHumanPawn(NewPlayer, nullptr))
	{
		if (PC->GetPawn() != Human)
		{
			PC->Possess(Human);
		}
	}
}

AShipCrewCharacter* AShipGameMode::SpawnOrRetrieveHumanPawn(AController* NewPlayer, AActor* StartSpot)
{
	EnsureCrewSpawned();

	if (!CrewPawnClass || !GetWorld())
	{
		return nullptr;
	}

	if (MandatoryRoles.Num() == 0)
	{
		return nullptr;
	}

	PlayerRoleSlotIndex = FMath::Clamp(PlayerRoleSlotIndex, 0, MandatoryRoles.Num() - 1);

	if (CrewPawns.IsValidIndex(PlayerRoleSlotIndex) && CrewPawns[PlayerRoleSlotIndex])
	{
		return CrewPawns[PlayerRoleSlotIndex];
	}

	FVector Loc(0.f, 0.f, 500.f);
	if (StartSpot)
	{
		Loc = StartSpot->GetActorLocation() + FVector(0.f, 0.f, 92.f);
	}
	else if (AActor* PS = UGameplayStatics::GetActorOfClass(GetWorld(), APlayerStart::StaticClass()))
	{
		Loc = PS->GetActorLocation() + FVector(0.f, 0.f, 92.f);
	}

	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AShipCrewCharacter* Human = GetWorld()->SpawnActor<AShipCrewCharacter>(CrewPawnClass, FTransform(Loc), P);
	if (!Human)
	{
		UE_LOG(LogSpaceshipCrew, Error, TEXT("ShipGameMode: аварийный спавн игрока не удался (класс %s)."), *GetNameSafe(CrewPawnClass.Get()));
		return nullptr;
	}

	if (CrewPawns.Num() <= PlayerRoleSlotIndex)
	{
		CrewPawns.SetNum(MandatoryRoles.Num());
	}
	CrewPawns[PlayerRoleSlotIndex] = Human;
	FinalizeCrewPawnSpawn(Human, PlayerRoleSlotIndex);
	RebuildGameStateCrewSlots();
	UE_LOG(LogSpaceshipCrew, Warning, TEXT("ShipGameMode: слот игрока %d был пуст — пешка создана аварийно у %s."),
		PlayerRoleSlotIndex, *Loc.ToString());
	return Human;
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

	// Капитан: все посты (MVP / удобство тестов; манифест в Game Mode может переопределить).
	MandatoryRoles.Add(MakeRole(FName(TEXT("Captain")), TArray<FName>{
		FName(TEXT("Helm")), FName(TEXT("Reactor")), FName(TEXT("Medical")), FName(TEXT("Extinguisher"))
	}));
	MandatoryRoles.Add(MakeRole(FName(TEXT("Engineer")), TArray<FName>{FName(TEXT("Reactor")), FName(TEXT("Extinguisher"))}));
	MandatoryRoles.Add(MakeRole(FName(TEXT("Medic")), TArray<FName>{FName(TEXT("Medical"))}));
	MandatoryRoles.Add(MakeRole(FName(TEXT("Pilot")), TArray<FName>{FName(TEXT("Helm"))}));
}

AShipActor* AShipGameMode::FindShipActor() const
{
	if (RuntimeSpawnedShip)
	{
		return RuntimeSpawnedShip;
	}
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AShipActor> It(World); It; ++It)
		{
			return *It;
		}
	}
	return nullptr;
}

void AShipGameMode::EnsureShipSpawnedAndConfigured()
{
	AShipActor* Ship = FindShipActor();
	if (!Ship && GetWorld() && ShipActorClass)
	{
		FVector SpawnLoc(0.f, 0.f, 120.f);
		FRotator SpawnRot = FRotator::ZeroRotator;
		if (AActor* Start = UGameplayStatics::GetActorOfClass(GetWorld(), APlayerStart::StaticClass()))
		{
			SpawnLoc = Start->GetActorLocation();
			SpawnRot = Start->GetActorRotation();
		}

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Ship = GetWorld()->SpawnActor<AShipActor>(ShipActorClass, SpawnLoc, SpawnRot, SpawnParams);
		RuntimeSpawnedShip = Ship;
	}

	if (Ship)
	{
		ApplyShipConfigSelection(Ship);
	}
}

void AShipGameMode::BuildCrewSpawnTransforms(AShipActor* Ship, const FVector& FallbackLocation, TArray<FTransform>& OutPerSlot) const
{
	BuildCrewSpawnTransformsInternal(MandatoryRoles, Ship, FallbackLocation, OutPerSlot);
}

void AShipGameMode::ApplyShipConfigSelection(AShipActor* Ship)
{
	if (!Ship)
	{
		return;
	}

	UShipConfigAsset* ConfigToApply = SelectedShipConfig.IsNull() ? nullptr : SelectedShipConfig.LoadSynchronous();
	if (!ConfigToApply && !Ship->DefaultShipConfig.IsNull())
	{
		ConfigToApply = Ship->DefaultShipConfig.LoadSynchronous();
	}

	if (!ConfigToApply)
	{
		return;
	}

	if (!Ship->ApplyShipConfigAsset(ConfigToApply))
	{
		UE_LOG(LogSpaceshipCrew, Warning, TEXT("ShipGameMode: failed to apply selected ship config '%s'"), *GetNameSafe(ConfigToApply));
	}
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
			Spawned->CrewBotBrain->RefreshBrainTimer();
		}
	}
	else
	{
		if (Spawned->CrewBotBrain)
		{
			Spawned->CrewBotBrain->bBrainEnabled = true;
			Spawned->CrewBotBrain->RefreshBrainTimer();
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

	TArray<FTransform> SlotTransforms;
	BuildCrewSpawnTransforms(Ship, Fallback, SlotTransforms);

	for (int32 i = 0; i < MandatoryRoles.Num(); ++i)
	{
		const FTransform Xform = SlotTransforms.IsValidIndex(i) ? SlotTransforms[i] : FTransform(FRotator::ZeroRotator, Fallback);
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
			UE_LOG(LogSpaceshipCrew, Warning, TEXT("ShipGameMode: слот игрока %d заспавнен у PlayerStart (резерв): проверьте CrewSpawnMarker / Z над палубой."),
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
	if (!Cast<APlayerController>(NewPlayer))
	{
		return Super::SpawnDefaultPawnFor_Implementation(NewPlayer, StartSpot);
	}
	return SpawnOrRetrieveHumanPawn(NewPlayer, StartSpot);
}

void AShipGameMode::PossessCrewSlot(APlayerController* Player, int32 SlotIndex)
{
	// В будущем: переносить игрока в SlotIndex и снимать управление с бота в этом слоте.
}

