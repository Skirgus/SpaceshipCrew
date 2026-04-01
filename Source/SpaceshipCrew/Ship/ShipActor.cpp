// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShipActor.h"
#include "ShipConfigAsset.h"
#include "ShipDoorActor.h"
#include "ShipModuleDefinition.h"
#include "ShipSystemsComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"

namespace
{
struct FResolvedModule
{
	const FShipModuleConfig* Config = nullptr;
	const UShipModuleDefinition* Definition = nullptr;
	FVector Center = FVector::ZeroVector;
	FVector Extent = FVector::ZeroVector;
};

FVector FaceNormal(EShipModuleFace Face)
{
	switch (Face)
	{
	case EShipModuleFace::PosX: return FVector::ForwardVector;
	case EShipModuleFace::NegX: return -FVector::ForwardVector;
	case EShipModuleFace::PosY: return FVector::RightVector;
	case EShipModuleFace::NegY: return -FVector::RightVector;
	case EShipModuleFace::PosZ: return FVector::UpVector;
	case EShipModuleFace::NegZ: return -FVector::UpVector;
	default: return FVector::ForwardVector;
	}
}

bool AreConnected(const UShipConfigAsset* Config, FName A, FName B)
{
	if (!Config)
	{
		return false;
	}
	return Config->Connections.ContainsByPredicate([A, B](const FShipConnectionConfig& C)
	{
		const bool bSame = C.ModuleA == A && C.ModuleB == B;
		const bool bOpposite = C.ModuleA == B && C.ModuleB == A;
		return bSame || bOpposite;
	});
}

bool TryGetConnectionState(const UShipConfigAsset* Config, FName A, FName B, bool& bOutOpen)
{
	if (!Config)
	{
		return false;
	}

	for (const FShipConnectionConfig& C : Config->Connections)
	{
		const bool bSame = C.ModuleA == A && C.ModuleB == B;
		const bool bOpposite = C.ModuleA == B && C.ModuleB == A;
		if (bSame || bOpposite)
		{
			bOutOpen = C.bOpenByDefault;
			return true;
		}
	}
	return false;
}

const FResolvedModule* FindConnectedNeighborOnFace(
	const UShipConfigAsset* Config,
	const TMap<FName, FResolvedModule>& ModulesById,
	FName SelfId,
	const FResolvedModule& SelfModule,
	const FVector& FaceNormalDir)
{
	for (const TPair<FName, FResolvedModule>& OtherPair : ModulesById)
	{
		if (OtherPair.Key == SelfId || !OtherPair.Value.Definition || !OtherPair.Value.Config)
		{
			continue;
		}
		if (!AreConnected(Config, SelfId, OtherPair.Key))
		{
			continue;
		}

		const FVector Delta = OtherPair.Value.Center - SelfModule.Center;
		const FVector DeltaDir = Delta.GetSafeNormal();
		if (FVector::DotProduct(DeltaDir, FaceNormalDir) < 0.7f)
		{
			continue;
		}
		return &OtherPair.Value;
	}
	return nullptr;
}
}

AShipActor::AShipActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(false);

	HullMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HullMesh"));
	RootComponent = HullMesh;

	ShipSystems = CreateDefaultSubobject<UShipSystemsComponent>(TEXT("ShipSystems"));
	DoorActorClass = AShipDoorActor::StaticClass();
}

void AShipActor::BeginPlay()
{
	Super::BeginPlay();
	if (ShipSystems)
	{
		ShipSystems->OnBulkheadsChanged.AddUObject(this, &AShipActor::HandleBulkheadsChanged);
	}
}

void AShipActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ShipSystems)
	{
		ShipSystems->OnBulkheadsChanged.RemoveAll(this);
	}
	Super::EndPlay(EndPlayReason);
}

bool AShipActor::ApplyShipConfigAsset(UShipConfigAsset* ShipConfigAsset)
{
	if (!ShipSystems || !ShipConfigAsset)
	{
		return false;
	}
	const bool bApplied = ShipSystems->ApplyShipConfig(ShipConfigAsset);
	if (bApplied)
	{
		RebuildModuleVisualsFromConfig(ShipConfigAsset);
	}
	return bApplied;
}

void AShipActor::RebuildModuleVisualsFromConfig(const UShipConfigAsset* ShipConfigAsset)
{
	for (UStaticMeshComponent* MeshComp : RuntimeModuleMeshes)
	{
		if (IsValid(MeshComp))
		{
			MeshComp->DestroyComponent();
		}
	}
	RuntimeModuleMeshes.Reset();
	for (UPointLightComponent* LightComp : RuntimeModuleLights)
	{
		if (IsValid(LightComp))
		{
			LightComp->DestroyComponent();
		}
	}
	RuntimeModuleLights.Reset();
	for (AShipDoorActor* Door : RuntimeDoorActors)
	{
		if (IsValid(Door))
		{
			Door->Destroy();
		}
	}
	RuntimeDoorActors.Reset();
	RuntimeDoorsByBulkheadKey.Reset();

	if (!ShipConfigAsset || !RootComponent)
	{
		return;
	}

	UStaticMesh* DebugCubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!DebugCubeMesh)
	{
		return;
	}

	FVector GridCenter = FVector::ZeroVector;
	if (bCenterModuleGridAroundActor && ShipConfigAsset->Modules.Num() > 0)
	{
		FVector Sum = FVector::ZeroVector;
		for (const FShipModuleConfig& ModuleConfig : ShipConfigAsset->Modules)
		{
			Sum += FVector(
				static_cast<float>(ModuleConfig.GridPosition.X),
				static_cast<float>(ModuleConfig.GridPosition.Y),
				static_cast<float>(ModuleConfig.GridPosition.Z)
			);
		}
		GridCenter = Sum / static_cast<float>(ShipConfigAsset->Modules.Num());
	}

	TMap<FName, FResolvedModule> ModulesById;
	for (const FShipModuleConfig& ModuleConfig : ShipConfigAsset->Modules)
	{
		const UShipModuleDefinition* Definition = ModuleConfig.ModuleDefinition.LoadSynchronous();
		if (!Definition)
		{
			continue;
		}

		const FVector GridPoint(
			static_cast<float>(ModuleConfig.GridPosition.X),
			static_cast<float>(ModuleConfig.GridPosition.Y),
			static_cast<float>(ModuleConfig.GridPosition.Z)
		);

		FResolvedModule Resolved;
		Resolved.Config = &ModuleConfig;
		Resolved.Definition = Definition;
		Resolved.Center = (GridPoint - GridCenter) * ModuleGridSize;
		Resolved.Extent = FVector(
			FMath::Max(1, Definition->GridSize.X) * ModuleGridSize * 0.5f,
			FMath::Max(1, Definition->GridSize.Y) * ModuleGridSize * 0.5f,
			FMath::Max(1, Definition->GridSize.Z) * ModuleGridSize * 0.5f
		);
		ModulesById.Add(ModuleConfig.ModuleId, Resolved);
	}

	int32 Index = 0;
	for (const TPair<FName, FResolvedModule>& Pair : ModulesById)
	{
		const FResolvedModule& Module = Pair.Value;
		if (!Module.Config || !Module.Definition)
		{
			continue;
		}

		auto SpawnPiece = [&](const FVector& LocalCenter, const FVector& LocalScale)
		{
			const FName ComponentName = FName(*FString::Printf(TEXT("RuntimeModule_%d"), Index++));
			UStaticMeshComponent* PieceComp = NewObject<UStaticMeshComponent>(this, ComponentName);
			if (!PieceComp)
			{
				return;
			}
			PieceComp->SetStaticMesh(DebugCubeMesh);
			PieceComp->SetupAttachment(RootComponent);
			PieceComp->SetMobility(EComponentMobility::Movable);
			PieceComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			PieceComp->SetCollisionResponseToAllChannels(ECR_Block);
			PieceComp->SetCanEverAffectNavigation(true);
			PieceComp->RegisterComponent();
			PieceComp->SetRelativeLocation(LocalCenter);
			PieceComp->SetWorldScale3D(LocalScale / 100.f); // Cube.Cube is 100 units
			RuntimeModuleMeshes.Add(PieceComp);
		};

		const FVector E = Module.Extent;
		const FVector C = Module.Center;

		if (!Module.Definition->bHasInteriorVolume)
		{
			// Внешний/технический модуль без комнаты.
			SpawnPiece(C, E * 2.f);
			continue;
		}

		UPointLightComponent* InteriorLight = NewObject<UPointLightComponent>(this);
		if (InteriorLight)
		{
			InteriorLight->SetupAttachment(RootComponent);
			InteriorLight->SetRelativeLocation(C + FVector(0.f, 0.f, FMath::Max(40.f, E.Z * 0.2f)));
			InteriorLight->SetIntensity(InteriorLightIntensity);
			InteriorLight->SetAttenuationRadius(InteriorLightRadius);
			InteriorLight->SetCastShadows(false);
			InteriorLight->RegisterComponent();
			RuntimeModuleLights.Add(InteriorLight);
		}

		// Комната: пол + потолок + "панели" в точках соединения.
		const float WallThickness = FMath::Clamp(ModuleGridSize * 0.08f, 12.f, 35.f);
		const float PanelSpan = FMath::Clamp(ModuleGridSize * 0.45f, 40.f, ModuleGridSize * 0.9f);

		SpawnPiece(C + FVector(0.f, 0.f, -E.Z + WallThickness * 0.5f), FVector(E.X * 2.f, E.Y * 2.f, WallThickness));
		SpawnPiece(C + FVector(0.f, 0.f, E.Z - WallThickness * 0.5f), FVector(E.X * 2.f, E.Y * 2.f, WallThickness));

		const TArray<FShipModuleConnectionPoint> Points = Module.Definition->GetEffectiveConnectionPoints();
		for (const FShipModuleConnectionPoint& Point : Points)
		{
			const FVector Normal = FaceNormal(Point.Face);
			FVector TangentA = FVector::RightVector;
			if (Point.Face == EShipModuleFace::PosY || Point.Face == EShipModuleFace::NegY)
			{
				TangentA = FVector::ForwardVector;
			}
			const float AxisLen = (Point.Face == EShipModuleFace::PosX || Point.Face == EShipModuleFace::NegX) ? E.Y : E.X;
			const float SlotOffset = FMath::Lerp(-AxisLen, AxisLen, FMath::Clamp(Point.Slot, 0.f, 1.f));

			const FVector FaceOffset(
				Normal.X * E.X,
				Normal.Y * E.Y,
				Normal.Z * E.Z
			);
			const FVector PanelCenter = C + FaceOffset + TangentA * SlotOffset;
			const FResolvedModule* Neighbor = FindConnectedNeighborOnFace(
				ShipConfigAsset,
				ModulesById,
				Pair.Key,
				Module,
				Normal
			);

			const bool bHasConnectedRoomOnFace = (Neighbor && Neighbor->Definition && Neighbor->Definition->bHasInteriorVolume);
			bool bConnectionOpen = true;
			if (Neighbor)
			{
				TryGetConnectionState(ShipConfigAsset, Pair.Key, Neighbor->Config->ModuleId, bConnectionOpen);
			}

			if (bHasConnectedRoomOnFace)
			{
				const float DoorWidth = FMath::Clamp(ModuleGridSize * 0.35f, 60.f, ModuleGridSize * 0.75f);
				const float DoorHeight = FMath::Clamp(E.Z * 2.f * 0.65f, 120.f, E.Z * 2.f * 0.9f);
				FVector DoorSize(DoorWidth, WallThickness, DoorHeight);
				if (Point.Face == EShipModuleFace::PosX || Point.Face == EShipModuleFace::NegX)
				{
					DoorSize = FVector(WallThickness, DoorWidth, DoorHeight);
				}

				const FVector TopCenter = PanelCenter + FVector(0.f, 0.f, E.Z * 0.5f);
				const float TopHeight = FMath::Max(10.f, E.Z * 2.f * 0.8f - DoorHeight);
				FVector TopSize(DoorSize.X, DoorSize.Y, TopHeight);
				SpawnPiece(TopCenter, TopSize);

				if (GetWorld())
				{
					FActorSpawnParameters SpawnParams;
					SpawnParams.Owner = this;
					SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
					const FRotator DoorRot = Normal.Rotation();
					UClass* DoorClassToSpawn = DoorActorClass ? DoorActorClass.Get() : AShipDoorActor::StaticClass();
					AShipDoorActor* Door = GetWorld()->SpawnActor<AShipDoorActor>(
						DoorClassToSpawn,
						GetActorLocation() + PanelCenter,
						DoorRot,
						SpawnParams);
					if (Door)
					{
						Door->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
						Door->SetActorScale3D(DoorSize / 100.f);
						Door->SetDoorOpen(bConnectionOpen);
						RuntimeDoorActors.Add(Door);
						const FString BulkheadKey = MakeBulkheadKey(Pair.Key, Neighbor->Config->ModuleId);
						RuntimeDoorsByBulkheadKey.FindOrAdd(BulkheadKey).Add(Door);
					}
				}
			}
			else
			{
				FVector PanelSize(PanelSpan, WallThickness, E.Z * 2.f * 0.8f);
				if (Point.Face == EShipModuleFace::PosX || Point.Face == EShipModuleFace::NegX)
				{
					PanelSize = FVector(WallThickness, PanelSpan, E.Z * 2.f * 0.8f);
				}
				SpawnPiece(PanelCenter, PanelSize);
			}
		}
	}

	SyncRuntimeDoorsFromBulkheads();
}

FString AShipActor::MakeBulkheadKey(FName ModuleA, FName ModuleB)
{
	const FString A = ModuleA.ToString();
	const FString B = ModuleB.ToString();
	return (A <= B) ? (A + TEXT("|") + B) : (B + TEXT("|") + A);
}

void AShipActor::SyncRuntimeDoorsFromBulkheads()
{
	if (!ShipSystems || RuntimeDoorsByBulkheadKey.Num() == 0)
	{
		return;
	}

	for (const FShipBulkheadState& Bulkhead : ShipSystems->Bulkheads)
	{
		const FString Key = MakeBulkheadKey(Bulkhead.ModuleA, Bulkhead.ModuleB);
		const TArray<TObjectPtr<AShipDoorActor>>* Doors = RuntimeDoorsByBulkheadKey.Find(Key);
		if (!Doors)
		{
			continue;
		}
		for (AShipDoorActor* Door : *Doors)
		{
			if (IsValid(Door))
			{
				Door->SetDoorOpen(Bulkhead.bOpen);
			}
		}
	}
}

void AShipActor::HandleBulkheadsChanged()
{
	SyncRuntimeDoorsFromBulkheads();
}
