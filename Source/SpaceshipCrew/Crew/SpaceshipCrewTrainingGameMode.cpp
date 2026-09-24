#include "SpaceshipCrewTrainingGameMode.h"

#include "EquippableItem.h"
#include "ShipBuilder/ShipBlueprintRegistry.h"
#include "ShipBuilder/ShipBlueprintTypes.h"
#include "ShipBuilder/ShipBuilderDomainGlue.h"
#include "ShipBuilder/ShipBuilderGridConstants.h"
#include "ShipBuilder/ShipBuilderModulePreviewActor.h"
#include "ShipModule/ShipModuleCatalog.h"
#include "ShipModule/ShipModuleDefinition.h"
#include "SpaceshipCrewCharacter.h"
#include "SpaceshipCrewTrainingPlayerController.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/GameInstance.h"
#include "Engine/PointLight.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	constexpr float FloorTopLocalZ = -184.0f;
	constexpr float CapsuleHalfHeight = 96.0f;

	template <typename T>
	T* FindOrNull(UWorld* World)
	{
		return World ? Cast<T>(UGameplayStatics::GetActorOfClass(World, T::StaticClass())) : nullptr;
	}

	void SpawnPointLamp(UWorld* World, const FVector& Location, float Intensity, float Attenuation)
	{
		APointLight* Lamp = World->SpawnActor<APointLight>(Location, FRotator::ZeroRotator);
		if (!Lamp)
		{
			return;
		}
		if (UPointLightComponent* Comp = Lamp->PointLightComponent)
		{
			Comp->SetMobility(EComponentMobility::Movable);
			Comp->SetIntensity(Intensity);
			Comp->SetAttenuationRadius(Attenuation);
			Comp->SetCastShadows(false);
		}
	}

	/** Убрать ad-hoc hull, tech-bay, станции инженера и декоративный груз. */
	void DestroyLegacyTrainingGeometry(UWorld* World)
	{
		auto DestroyByTag = [World](FName Tag)
		{
			TArray<AActor*> Actors;
			UGameplayStatics::GetAllActorsWithTag(World, Tag, Actors);
			for (AActor* Actor : Actors)
			{
				if (Actor)
				{
					Actor->Destroy();
				}
			}
		};
		DestroyByTag(FName(TEXT("ET_TrainingHull")));
		DestroyByTag(FName(TEXT("ET_CollisionFloor")));

		TArray<UStaticMesh*> PropMeshes;
		auto Collect = [&PropMeshes](const TCHAR* Dir, const TCHAR* Name)
		{
			const FString Path = FString::Printf(TEXT("%s/%s.%s"), Dir, Name, Name);
			if (UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *Path))
			{
				PropMeshes.Add(Mesh);
			}
		};
		Collect(TEXT("/Game/Meshes/EngineerTraining"), TEXT("SM_TechBay_Interior_1x1x1"));
		Collect(TEXT("/Game/Meshes/EngineerTraining"), TEXT("SM_EnergyConsole"));
		Collect(TEXT("/Game/Meshes/EngineerTraining"), TEXT("SM_HullPanel_Damaged"));
		Collect(TEXT("/Game/Meshes/EngineerTraining"), TEXT("SM_HullPanel_Intact"));
		Collect(TEXT("/Game/Meshes/EngineerTraining"), TEXT("SM_RepairTorch"));
		Collect(TEXT("/Game/Meshes/TrainingShip"), TEXT("SM_Cargo_Pad"));
		for (int32 i = 0; i < 5; ++i)
		{
			Collect(TEXT("/Game/Meshes/TrainingShip"), *FString::Printf(TEXT("SM_Cargo_Crate_%d"), i));
		}

		TArray<AActor*> MeshActors;
		UGameplayStatics::GetAllActorsOfClass(World, AStaticMeshActor::StaticClass(), MeshActors);
		for (AActor* Actor : MeshActors)
		{
			AStaticMeshActor* SMA = Cast<AStaticMeshActor>(Actor);
			if (!SMA || !SMA->GetStaticMeshComponent())
			{
				continue;
			}
			UStaticMesh* Mesh = SMA->GetStaticMeshComponent()->GetStaticMesh();
			if (Mesh && PropMeshes.Contains(Mesh))
			{
				SMA->Destroy();
			}
		}
	}
}

ASpaceshipCrewTrainingGameMode::ASpaceshipCrewTrainingGameMode()
{
	DefaultPawnClass = ASpaceshipCrewCharacter::StaticClass();
	PlayerControllerClass = ASpaceshipCrewTrainingPlayerController::StaticClass();
	DemoPickupItemClass = TSoftClassPtr<AEquippableItem>(
		FSoftObjectPath(TEXT("/Game/Blueprints/Items/BP_RepairTorch.BP_RepairTorch_C")));
}

void ASpaceshipCrewTrainingGameMode::InitGame(
	const FString& MapName,
	const FString& Options,
	FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	EnsureTrainingLayout();
}

AActor* ASpaceshipCrewTrainingGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	if (IsValid(TrainingPlayerStart))
	{
		return TrainingPlayerStart;
	}
	return Super::ChoosePlayerStart_Implementation(Player);
}

void ASpaceshipCrewTrainingGameMode::RestartPlayer(AController* NewPlayer)
{
	Super::RestartPlayer(NewPlayer);

	if (!NewPlayer)
	{
		return;
	}
	if (APawn* Pawn = NewPlayer->GetPawn())
	{
		const FTransform& Xform = TrainingSpawnTransform;
		Pawn->SetActorTransform(Xform, false, nullptr, ETeleportType::ResetPhysics);
		if (AController* C = Pawn->GetController())
		{
			C->SetControlRotation(Xform.Rotator());
		}
		UE_LOG(LogTemp, Log, TEXT("EngineerTraining: pawn on TrainingVessel at %s"),
			*Xform.GetLocation().ToString());
	}

	SpawnDemoPickupNearStart();
}

void ASpaceshipCrewTrainingGameMode::SpawnDemoPickupNearStart()
{
	if (bDemoPickupSpawned)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UClass* ItemClass = DemoPickupItemClass.LoadSynchronous();
	if (!ItemClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("EngineerTraining: demo pickup class missing (%s)"),
			*DemoPickupItemClass.ToString());
		return;
	}

	const FVector SpawnLoc = TrainingSpawnTransform.GetLocation()
		+ TrainingSpawnTransform.GetRotation().RotateVector(FVector(120.0f, 40.0f, 20.0f));
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (AEquippableItem* Item = World->SpawnActor<AEquippableItem>(
			ItemClass, SpawnLoc, TrainingSpawnTransform.Rotator(), Params))
	{
		bDemoPickupSpawned = true;
		UE_LOG(LogTemp, Log, TEXT("EngineerTraining: spawned demo pickup %s at %s"),
			*Item->GetName(), *SpawnLoc.ToString());
	}
}

bool ASpaceshipCrewTrainingGameMode::SpawnTrainingShipHull()
{
	UWorld* World = GetWorld();
	UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
	if (!GI)
	{
		UE_LOG(LogTemp, Error, TEXT("EngineerTraining: no GameInstance"));
		return false;
	}

	UShipBlueprintRegistry* Registry = GI->GetSubsystem<UShipBlueprintRegistry>();
	UShipModuleCatalog* Catalog = GI->GetSubsystem<UShipModuleCatalog>();
	if (!Registry || !Catalog)
	{
		UE_LOG(LogTemp, Error, TEXT("EngineerTraining: Registry/Catalog missing"));
		return false;
	}

	Registry->EnsureProjectCatalogFresh();

	FShipBlueprintDocument Document;
	FString LoadError;
	if (!Registry->LoadDocument(EShipBlueprintSource::Project, TrainingShipId, Document, LoadError))
	{
		UE_LOG(LogTemp, Error, TEXT("EngineerTraining: failed to load ship '%s': %s"),
			*TrainingShipId.ToString(), *LoadError);
		return false;
	}

	// Пока EngineeringBay_01 не импортирован — подмена на коридор 1×1 в той же ячейке.
	for (FShipBuilderPlacedModule& Placed : Document.ModuleLayout.PlacedModules)
	{
		if (Catalog->FindModuleById(Placed.ModuleId))
		{
			continue;
		}
		UE_LOG(LogTemp, Warning,
			TEXT("EngineerTraining: module '%s' missing — fallback Corridor_CustomPanels_01"),
			*Placed.ModuleId.ToString());
		Placed.ModuleId = FName(TEXT("Corridor_CustomPanels_01"));
	}
	for (FName& ModuleId : Document.ModuleLayout.ModuleIds)
	{
		if (!Catalog->FindModuleById(ModuleId))
		{
			ModuleId = FName(TEXT("Corridor_CustomPanels_01"));
		}
	}

	auto Resolve = [Catalog](FName ModuleId) -> const UShipModuleDefinition*
	{
		return Catalog->FindModuleById(ModuleId);
	};

	if (Document.ModuleLayout.Connections.Num() == 0 && Document.ModuleLayout.PlacedModules.Num() >= 2)
	{
		SpaceshipCrew_RebuildDraftConnectionsFromAdjacency(
			Document.ModuleLayout,
			Resolve,
			ShipBuilderGrid::PanelUnitXY,
			ShipBuilderGrid::PanelUnitZ);
	}

	TrainingHull = World->SpawnActor<AShipBuilderModulePreviewActor>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (!TrainingHull)
	{
		UE_LOG(LogTemp, Error, TEXT("EngineerTraining: failed to spawn hull actor"));
		return false;
	}

	TrainingHull->SetPreviewDamageEnabled(false);
	TrainingHull->RebuildFromDraft(Document.ModuleLayout, *Catalog);

	EngineeringModuleCenter = FVector(200.0f, 200.0f, 150.0f);
	for (const FShipBuilderPlacedModule& Placed : Document.ModuleLayout.PlacedModules)
	{
		if (Placed.InstanceId != EngineeringModuleInstanceId)
		{
			continue;
		}
		if (const UShipModuleDefinition* Def = Catalog->FindModuleById(Placed.ModuleId))
		{
			EngineeringModuleCenter = SpaceshipCrew_ComputeModuleWorldCenter(
				Placed,
				*Def,
				ShipBuilderGrid::PanelUnitXY,
				ShipBuilderGrid::PanelUnitZ);
		}
		break;
	}

	const float FloorWorldZ = EngineeringModuleCenter.Z + FloorTopLocalZ;
	TrainingSpawnTransform = FTransform(
		FRotator(0.0f, 180.0f, 0.0f),
		FVector(EngineeringModuleCenter.X, EngineeringModuleCenter.Y, FloorWorldZ + CapsuleHalfHeight));

	UE_LOG(LogTemp, Log,
		TEXT("EngineerTraining: TrainingVessel spawned (%d modules), engineering center %s"),
		Document.ModuleLayout.PlacedModules.Num(),
		*EngineeringModuleCenter.ToString());
	return true;
}

void ASpaceshipCrewTrainingGameMode::PlaceTrainingPropsAround(const FVector& EngineeringCenter)
{
	UE_LOG(LogTemp, Log,
		TEXT("EngineerTraining: equipment comes from module placements (center=%s)"),
		*EngineeringCenter.ToString());
}

void ASpaceshipCrewTrainingGameMode::EnsureTrainingLayout()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	DestroyLegacyTrainingGeometry(World);

	if (!FindOrNull<ASkyAtmosphere>(World))
	{
		World->SpawnActor<ASkyAtmosphere>(FVector::ZeroVector, FRotator::ZeroRotator);
	}

	ADirectionalLight* Sun = FindOrNull<ADirectionalLight>(World);
	if (!Sun)
	{
		Sun = World->SpawnActor<ADirectionalLight>(
			FVector(0.0f, 0.0f, 800.0f), FRotator(-40.0f, 30.0f, 0.0f));
	}
	if (Sun)
	{
		if (UDirectionalLightComponent* SunComp = Cast<UDirectionalLightComponent>(Sun->GetLightComponent()))
		{
			SunComp->SetMobility(EComponentMobility::Movable);
			SunComp->SetIntensity(12.0f);
			SunComp->SetCastShadows(false);
		}
	}

	ASkyLight* Sky = FindOrNull<ASkyLight>(World);
	if (!Sky)
	{
		Sky = World->SpawnActor<ASkyLight>(FVector(0.0f, 0.0f, 600.0f), FRotator::ZeroRotator);
	}
	if (Sky)
	{
		if (USkyLightComponent* SkyComp = Cast<USkyLightComponent>(Sky->GetLightComponent()))
		{
			SkyComp->SetMobility(EComponentMobility::Movable);
			SkyComp->SetIntensity(1.2f);
			SkyComp->SetRealTimeCaptureEnabled(true);
		}
	}

	const bool bShipOk = SpawnTrainingShipHull();
	if (!bShipOk)
	{
		UE_LOG(LogTemp, Error, TEXT("EngineerTraining: TrainingVessel unavailable — check Content/Data/Ships and Corridor_CustomPanels_01"));
		TrainingSpawnTransform = FTransform(FRotator(0.0f, 180.0f, 0.0f), FVector(200.0f, 200.0f, 100.0f));
		EngineeringModuleCenter = FVector(200.0f, 200.0f, 150.0f);
	}

	if (!FindOrNull<APointLight>(World))
	{
		SpawnPointLamp(World, EngineeringModuleCenter + FVector(0.0f, 0.0f, 40.0f), 16000.0f, 1400.0f);
		SpawnPointLamp(World, EngineeringModuleCenter + FVector(-200.0f, 0.0f, 20.0f), 10000.0f, 1200.0f);
	}

	{
		TArray<AActor*> ExistingStarts;
		UGameplayStatics::GetAllActorsOfClass(World, APlayerStart::StaticClass(), ExistingStarts);
		APlayerStart* Chosen = nullptr;
		for (AActor* StartActor : ExistingStarts)
		{
			if (APlayerStart* PS = Cast<APlayerStart>(StartActor))
			{
				if (!Chosen)
				{
					Chosen = PS;
				}
				PS->SetActorTransform(TrainingSpawnTransform, false, nullptr, ETeleportType::ResetPhysics);
			}
		}
		if (!Chosen)
		{
			Chosen = World->SpawnActor<APlayerStart>(
				TrainingSpawnTransform.GetLocation(),
				TrainingSpawnTransform.Rotator());
		}
		TrainingPlayerStart = Chosen;
	}

	PlaceTrainingPropsAround(EngineeringModuleCenter);
}
