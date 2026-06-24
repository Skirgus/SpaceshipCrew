#include "ShipBuilder/ShipBuilderModulePreviewActor.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ShipBuilder/ShipBuilderDomainGlue.h"
#include "ShipBuilder/ShipBuilderGridConstants.h"
#include "ShipModule/ShipBuildDomain.h"
#include "ShipModule/ShipModuleCatalog.h"
#include "ShipModule/ShipModuleDefinition.h"
#include "ShipModule/ShipModuleTypes.h"
#include "ShipModule/ShipModuleVisualOverride.h"
#include "UObject/ConstructorHelpers.h"

namespace ShipBuilderPreviewActorPrivate
{
	static constexpr float GridStepXY = 400.0f;
	static constexpr float GridStepZ = 300.0f;

	static bool IsHorizontalDoorwaySocket(const UShipModuleDefinition& Def, const FName SocketName)
	{
		TArray<FShipModuleContactPoint> PlacementSockets;
		Def.GatherContactPointsForPlacement(PlacementSockets);
		for (const FShipModuleContactPoint& CP : PlacementSockets)
		{
			if (CP.SocketName == SocketName)
			{
				return CP.SocketType == EShipModuleSocketType::Horizontal
					|| CP.SocketType == EShipModuleSocketType::Universal;
			}
		}

		const FString Socket = SocketName.ToString().ToLower();
		return Socket.Contains(TEXT("front"))
			|| Socket.Contains(TEXT("back"))
			|| Socket.Contains(TEXT("rear"))
			|| Socket.Contains(TEXT("left"))
			|| Socket.Contains(TEXT("right"));
	}

	static bool IsVerticalDoorwaySocket(const UShipModuleDefinition& Def, const FName SocketName)
	{
		TArray<FShipModuleContactPoint> PlacementSockets;
		Def.GatherContactPointsForPlacement(PlacementSockets);
		for (const FShipModuleContactPoint& CP : PlacementSockets)
		{
			if (CP.SocketName == SocketName)
			{
				return CP.SocketType == EShipModuleSocketType::Vertical
					|| CP.SocketType == EShipModuleSocketType::Universal;
			}
		}

		const FString Socket = SocketName.ToString().ToLower();
		return Socket.Contains(TEXT("top")) || Socket.Contains(TEXT("bottom"));
	}

	static bool FindSocketRelativeLocation(const UShipModuleDefinition& Def, const FName SocketName, FVector& OutLocation)
	{
		TArray<FShipModuleContactPoint> PlacementSockets;
		Def.GatherContactPointsForPlacement(PlacementSockets);
		for (const FShipModuleContactPoint& CP : PlacementSockets)
		{
			if (CP.SocketName == SocketName)
			{
				OutLocation = CP.RelativeLocation;
				return true;
			}
		}

		for (const FShipModuleContactPoint& CP : PlacementSockets)
		{
			if (SpaceshipCrew_DoPanelSocketsMatch(SocketName, CP.SocketName))
			{
				OutLocation = CP.RelativeLocation;
				return true;
			}
		}

		return false;
	}

	static FVector DoorFrameCenterOnWallPlane(const FVector& SocketLocation, const FVector& Half, const float T)
	{
		if (FMath::IsNearlyEqual(FMath::Abs(SocketLocation.X), Half.X, 1.0f))
		{
			return FVector(FMath::Sign(SocketLocation.X) * (Half.X - T * 0.5f), SocketLocation.Y, SocketLocation.Z);
		}
		if (FMath::IsNearlyEqual(FMath::Abs(SocketLocation.Y), Half.Y, 1.0f))
		{
			return FVector(SocketLocation.X, FMath::Sign(SocketLocation.Y) * (Half.Y - T * 0.5f), SocketLocation.Z);
		}
		return SocketLocation;
	}

	static FName MakePanelSocketName(const TCHAR* FacePrefix, const int32 IX, const int32 IY, const int32 IZ)
	{
		return FName(*FString::Printf(TEXT("%s_X%d_Y%d_Z%d"), FacePrefix, IX, IY, IZ));
	}

	static FName ForcedOpeningSocketForSide(const FIntVector& Cells, const EShipModuleOpeningSide Side)
	{
		const int32 MidY = FMath::Max(0, (Cells.Y - 1) / 2);
		const int32 MidZ = FMath::Max(0, (Cells.Z - 1) / 2);
		const int32 MidX = FMath::Max(0, (Cells.X - 1) / 2);
		switch (Side)
		{
		case EShipModuleOpeningSide::Front:
			return MakePanelSocketName(TEXT("Front"), Cells.X - 1, MidY, MidZ);
		case EShipModuleOpeningSide::Back:
			return MakePanelSocketName(TEXT("Back"), 0, MidY, MidZ);
		case EShipModuleOpeningSide::Left:
			return MakePanelSocketName(TEXT("Left"), MidX, 0, MidZ);
		case EShipModuleOpeningSide::Right:
			return MakePanelSocketName(TEXT("Right"), MidX, Cells.Y - 1, MidZ);
		default:
			return NAME_None;
		}
	}
}

AShipBuilderModulePreviewActor::AShipBuilderModulePreviewActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Static);
	SetRootComponent(Root);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (CubeMesh.Succeeded())
	{
		PanelMesh = CubeMesh.Object;
		DoorFrameMesh = CubeMesh.Object;
		DamagedPanelMesh = CubeMesh.Object;
		SolidModuleMesh = CubeMesh.Object;
	}
	if (BasicShapeMaterial.Succeeded())
	{
		SocketMarkerBaseMaterial = BasicShapeMaterial.Object;
	}
}

UInstancedStaticMeshComponent& AShipBuilderModulePreviewActor::GetOrCreatePool(
	TMap<TObjectPtr<UStaticMesh>, TObjectPtr<UInstancedStaticMeshComponent>>& Pools,
	UStaticMesh* Mesh,
	const TCHAR* NamePrefix)
{
	check(Mesh);
	if (TObjectPtr<UInstancedStaticMeshComponent>* Existing = Pools.Find(Mesh))
	{
		return *Existing->Get();
	}

	const FName ComponentName = *FString::Printf(TEXT("%s_%s"), NamePrefix, *Mesh->GetName());
	UInstancedStaticMeshComponent* NewComponent = NewObject<UInstancedStaticMeshComponent>(this, ComponentName);
	NewComponent->SetupAttachment(Root);
	NewComponent->SetMobility(EComponentMobility::Static);
	NewComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	NewComponent->SetCastShadow(true);
	NewComponent->SetStaticMesh(Mesh);
	NewComponent->RegisterComponent();

	Pools.Add(Mesh, NewComponent);
	return *NewComponent;
}

void AShipBuilderModulePreviewActor::ClearPools(
	TMap<TObjectPtr<UStaticMesh>, TObjectPtr<UInstancedStaticMeshComponent>>& Pools)
{
	for (TPair<TObjectPtr<UStaticMesh>, TObjectPtr<UInstancedStaticMeshComponent>>& Entry : Pools)
	{
		if (Entry.Value)
		{
			Entry.Value->ClearInstances();
		}
	}
}

void AShipBuilderModulePreviewActor::ConfigureSelectionComponent(UInstancedStaticMeshComponent& Component) const
{
	Component.SetRenderCustomDepth(true);
	Component.SetCustomDepthStencilValue(252);
}

void AShipBuilderModulePreviewActor::ConfigureSocketMarkerComponent(UInstancedStaticMeshComponent& Component) const
{
	Component.SetRenderCustomDepth(true);
	Component.SetCustomDepthStencilValue(253);
	if (SocketMarkerBaseMaterial && !Cast<UMaterialInstanceDynamic>(Component.GetMaterial(0)))
	{
		Component.SetMaterial(0, SocketMarkerBaseMaterial);
	}
	if (UMaterialInstanceDynamic* ExistingMid = Cast<UMaterialInstanceDynamic>(Component.GetMaterial(0)))
	{
		ExistingMid->SetVectorParameterValue(TEXT("Color"), SocketMarkerColor);
		ExistingMid->SetVectorParameterValue(TEXT("BaseColor"), SocketMarkerColor);
		ExistingMid->SetVectorParameterValue(TEXT("Tint"), SocketMarkerColor);
		ExistingMid->SetVectorParameterValue(TEXT("EmissiveColor"), SocketMarkerColor * 4.0f);
		return;
	}
	if (UMaterialInterface* BaseMaterial = Component.GetMaterial(0))
	{
		if (UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(BaseMaterial, const_cast<AShipBuilderModulePreviewActor*>(this)))
		{
			Mid->SetVectorParameterValue(TEXT("Color"), SocketMarkerColor);
			Mid->SetVectorParameterValue(TEXT("BaseColor"), SocketMarkerColor);
			Mid->SetVectorParameterValue(TEXT("Tint"), SocketMarkerColor);
			Mid->SetVectorParameterValue(TEXT("EmissiveColor"), SocketMarkerColor * 4.0f);
			Component.SetMaterial(0, Mid);
		}
	}
}

FVector AShipBuilderModulePreviewActor::GetDragGhostWorldCenter(const FVector& ModuleSize) const
{
	const FVector CornerWorld(
		static_cast<float>(DragGhostGridPos.X) * ShipBuilderGrid::PanelUnitXY,
		static_cast<float>(DragGhostGridPos.Y) * ShipBuilderGrid::PanelUnitXY,
		static_cast<float>(DragGhostGridPos.Z) * ShipBuilderGrid::PanelUnitZ);
	return CornerWorld + ModuleSize * 0.5f;
}

void AShipBuilderModulePreviewActor::AddBoxInstance(
	UInstancedStaticMeshComponent& Component,
	const FVector& Center,
	const FVector& Size) const
{
	const FVector Scale = Size / 100.0f;
	const FTransform Transform(FRotator::ZeroRotator, Center, Scale);
	AddTransformInstance(Component, Transform);
}

void AShipBuilderModulePreviewActor::AddEffectiveSocketMarkerInstances(
	const UShipModuleDefinition& Def,
	const FTransform& ModuleTransform,
	UInstancedStaticMeshComponent& SocketPool,
	const float MarkerSize,
	const float InSocketMarkerOffset) const
{
	TArray<FShipModuleContactPoint> Effective;
	Def.GatherContactPointsForPlacement(Effective);
	for (const FShipModuleContactPoint& CP : Effective)
	{
		const FVector N = CP.RelativeLocation.GetSafeNormal();
		const FVector Pos = ModuleTransform.TransformPosition(CP.RelativeLocation + N * InSocketMarkerOffset);
		AddBoxInstance(SocketPool, Pos, FVector(MarkerSize, MarkerSize, MarkerSize));
	}
}

void AShipBuilderModulePreviewActor::AddTransformInstance(
	UInstancedStaticMeshComponent& Component,
	const FTransform& Transform) const
{
	Component.AddInstance(Transform);
}

void AShipBuilderModulePreviewActor::AddShellPanel(
	UInstancedStaticMeshComponent& NormalComponent,
	UInstancedStaticMeshComponent* DamagedComponent,
	const FVector& Center,
	const FVector& Size,
	int32& InOutPanelOrdinal) const
{
	const bool bUseDamaged = bPreviewDamage
		&& DamageEveryNthPanel > 1
		&& DamagedComponent != nullptr
		&& ((InOutPanelOrdinal + 1) % DamageEveryNthPanel == 0);

	AddBoxInstance(bUseDamaged ? *DamagedComponent : NormalComponent, Center, Size);
	++InOutPanelOrdinal;
}

void AShipBuilderModulePreviewActor::AddRotatedSelectionOutline(
	UInstancedStaticMeshComponent& SelectionPool,
	const FRotator& ModuleYaw,
	const FVector& Center,
	const FVector& Size,
	const float Pad,
	const float Thickness) const
{
	const float OutlineTopZ = Size.Z * 0.5f + Thickness * 0.5f + Pad * 0.15f;
	const float OuterX = Size.X + 2.0f * Pad;
	const float OuterY = Size.Y + 2.0f * Pad;

	const auto AddBar = [&](const FVector& LocalCenter, const FVector& BarSize)
	{
		const FTransform BarTransform(
			ModuleYaw,
			Center + ModuleYaw.RotateVector(LocalCenter),
			BarSize / 100.0f);
		AddTransformInstance(SelectionPool, BarTransform);
	};

	AddBar(FVector(0.0f, -OuterY * 0.5f + Thickness * 0.5f, OutlineTopZ), FVector(OuterX, Thickness, Thickness));
	AddBar(FVector(0.0f, OuterY * 0.5f - Thickness * 0.5f, OutlineTopZ), FVector(OuterX, Thickness, Thickness));
	AddBar(FVector(-OuterX * 0.5f + Thickness * 0.5f, 0.0f, OutlineTopZ), FVector(Thickness, OuterY, Thickness));
	AddBar(FVector(OuterX * 0.5f - Thickness * 0.5f, 0.0f, OutlineTopZ), FVector(Thickness, OuterY, Thickness));
}

void AShipBuilderModulePreviewActor::AddDoorOpeningFrame(
	UInstancedStaticMeshComponent& FrameComponent,
	const FRotator& ModuleYaw,
	const FVector& ModuleCenter,
	const FVector& LocalWallCenter,
	const float WallThickness,
	const float WallSpan,
	const float WallHeight,
	const bool bNormalAlongX) const
{
	const float OpenWidth = FMath::Clamp(160.0f, 80.0f, FMath::Max(80.0f, WallSpan - 2.0f * WallThickness));
	const float OpenHeight = FMath::Clamp(220.0f, 120.0f, FMath::Max(120.0f, WallHeight - 2.0f * WallThickness));

	const auto AddFrameBox = [&](const FVector& LocalCenter, const FVector& Size)
	{
		const FTransform FrameTransform(
			ModuleYaw,
			ModuleCenter + ModuleYaw.RotateVector(LocalCenter),
			Size / 100.0f);
		AddTransformInstance(FrameComponent, FrameTransform);
	};

	const float SideWidth = (WallSpan - OpenWidth) * 0.5f;
	if (SideWidth > 1.0f)
	{
		if (bNormalAlongX)
		{
			AddFrameBox(
				LocalWallCenter + FVector(0.0f, -WallSpan * 0.5f + SideWidth * 0.5f, 0.0f),
				FVector(WallThickness, SideWidth, WallHeight));
			AddFrameBox(
				LocalWallCenter + FVector(0.0f, WallSpan * 0.5f - SideWidth * 0.5f, 0.0f),
				FVector(WallThickness, SideWidth, WallHeight));
		}
		else
		{
			AddFrameBox(
				LocalWallCenter + FVector(-WallSpan * 0.5f + SideWidth * 0.5f, 0.0f, 0.0f),
				FVector(SideWidth, WallThickness, WallHeight));
			AddFrameBox(
				LocalWallCenter + FVector(WallSpan * 0.5f - SideWidth * 0.5f, 0.0f, 0.0f),
				FVector(SideWidth, WallThickness, WallHeight));
		}
	}

	const float OpeningTopZ = -WallHeight * 0.5f + WallThickness + OpenHeight;
	const float TopHeight = WallHeight * 0.5f - OpeningTopZ;
	if (TopHeight > 1.0f)
	{
		if (bNormalAlongX)
		{
			AddFrameBox(
				LocalWallCenter + FVector(0.0f, 0.0f, OpeningTopZ + TopHeight * 0.5f),
				FVector(WallThickness, OpenWidth, TopHeight));
		}
		else
		{
			AddFrameBox(
				LocalWallCenter + FVector(0.0f, 0.0f, OpeningTopZ + TopHeight * 0.5f),
				FVector(OpenWidth, WallThickness, TopHeight));
		}
	}
}

void AShipBuilderModulePreviewActor::AddPanelHatchFrame(
	UInstancedStaticMeshComponent& FrameComponent,
	const FRotator& ModuleYaw,
	const FVector& ModuleCenter,
	const FVector& LocalPanelCenter,
	const float Thickness,
	const float HatchSpan) const
{
	const float OpenSize = FMath::Clamp(160.0f, 80.0f, FMath::Max(80.0f, HatchSpan - 2.0f * Thickness));
	const float SideWidth = (HatchSpan - OpenSize) * 0.5f;

	const auto AddFrameBox = [&](const FVector& LocalCenter, const FVector& Size)
	{
		const FTransform FrameTransform(
			ModuleYaw,
			ModuleCenter + ModuleYaw.RotateVector(LocalCenter),
			Size / 100.0f);
		AddTransformInstance(FrameComponent, FrameTransform);
	};

	if (SideWidth > 1.0f)
	{
		AddFrameBox(
			LocalPanelCenter + FVector(-HatchSpan * 0.5f + SideWidth * 0.5f, 0.0f, 0.0f),
			FVector(SideWidth, HatchSpan, Thickness));
		AddFrameBox(
			LocalPanelCenter + FVector(HatchSpan * 0.5f - SideWidth * 0.5f, 0.0f, 0.0f),
			FVector(SideWidth, HatchSpan, Thickness));
		AddFrameBox(
			LocalPanelCenter + FVector(0.0f, -HatchSpan * 0.5f + SideWidth * 0.5f, 0.0f),
			FVector(OpenSize, SideWidth, Thickness));
		AddFrameBox(
			LocalPanelCenter + FVector(0.0f, HatchSpan * 0.5f - SideWidth * 0.5f, 0.0f),
			FVector(OpenSize, SideWidth, Thickness));
	}
}

bool AShipBuilderModulePreviewActor::ShouldForceOpeningForSide(
	const UShipModuleDefinition& Definition,
	const EShipModuleOpeningSide Side)
{
	return Definition.ModuleType == EShipModuleType::Airlock
		&& Definition.ForcedOpeningSide == Side;
}

UStaticMesh* AShipBuilderModulePreviewActor::ResolveMeshForType(
	const TMap<EShipModuleType, TObjectPtr<UStaticMesh>>& Overrides,
	const UStaticMesh* DefaultMesh,
	const EShipModuleType ModuleType)
{
	if (const TObjectPtr<UStaticMesh>* OverrideMesh = Overrides.Find(ModuleType))
	{
		return OverrideMesh->Get();
	}
	return const_cast<UStaticMesh*>(DefaultMesh);
}

void AShipBuilderModulePreviewActor::RebuildFromDraft(
	const FShipBuilderDraftConfig& Draft,
	const UShipModuleCatalog& Catalog)
{
	if (!Root)
	{
		return;
	}

	ClearPools(PanelMeshPools);
	ClearPools(DamagedPanelMeshPools);
	ClearPools(DoorFrameMeshPools);
	ClearPools(SolidMeshPools);
	ClearPools(OverrideMeshPools);
	ClearPools(SelectionMeshPools);
	ClearPools(SocketMarkerPools);

	if (Draft.ModuleIds.Num() == 0)
	{
		return;
	}
	if (!PanelMesh)
	{
		return;
	}

	struct FResolvedModule
	{
		const UShipModuleDefinition* Def = nullptr;
		FVector Center = FVector::ZeroVector;
		FName InstanceId = NAME_None;
		int32 YawStep = 0;
	};

	TArray<FResolvedModule> Resolved;
	const int32 PlannedCount = Draft.PlacedModules.Num() > 0 ? Draft.PlacedModules.Num() : Draft.ModuleIds.Num();
	Resolved.Reserve(PlannedCount);

	FCatalogShipBuildModuleResolver Resolver(Catalog);
	FShipBuildDomainModel DomainModel(Resolver);
	FString BuildDomainError;
	const bool bHasDomainChain = SpaceshipCrew_BuildDomainFromDraftChain(Draft, Resolver, DomainModel, BuildDomainError);

	float CursorX = 0.0f;
	if (Draft.PlacedModules.Num() > 0)
	{
		for (const FShipBuilderDraftConfig::FPlacedModule& Placed : Draft.PlacedModules)
		{
			const UShipModuleDefinition* Def = Catalog.FindModuleById(Placed.ModuleId);
			if (!Def)
			{
				continue;
			}

			FResolvedModule Entry;
			Entry.Def = Def;
			Entry.InstanceId = Placed.InstanceId;
			Entry.YawStep = Placed.YawStep;
			Entry.Center = SpaceshipCrew_ComputeModuleWorldCenter(
				Placed,
				*Def,
				ShipBuilderPreviewActorPrivate::GridStepXY,
				ShipBuilderPreviewActorPrivate::GridStepZ);
			Resolved.Add(Entry);
		}
	}
	else
	{
		for (const FName ModuleId : Draft.ModuleIds)
		{
			const UShipModuleDefinition* Def = Catalog.FindModuleById(ModuleId);
			if (!Def)
			{
				continue;
			}

			FResolvedModule Entry;
			Entry.Def = Def;
			Entry.InstanceId = *FString::Printf(TEXT("Draft%d"), Resolved.Num());
			Entry.Center = FVector(CursorX + Def->Size.X * 0.5f, 0.0f, Def->Size.Z * 0.5f);
			Resolved.Add(Entry);
			CursorX += Def->Size.X + ModuleGap;
		}
	}

	int32 GlobalPanelOrdinal = 0;
	TMap<FName, int32> IndexByInstanceId;
	for (int32 i = 0; i < Resolved.Num(); ++i)
	{
		IndexByInstanceId.Add(Resolved[i].InstanceId, i);
	}
	for (int32 Index = 0; Index < Resolved.Num(); ++Index)
	{
		const UShipModuleDefinition* Def = Resolved[Index].Def;
		if (!Def)
		{
			continue;
		}

		const FVector Size = Def->Size.ComponentMax(FVector(20.0f, 20.0f, 20.0f));
		const bool bIsSelected = !SelectedModuleInstanceId.IsNone()
			&& SelectedModuleInstanceId == Resolved[Index].InstanceId;
		const bool bIsHovered = !HoveredModuleInstanceId.IsNone()
			&& HoveredModuleInstanceId == Resolved[Index].InstanceId;
		const bool bIsDragGhostTarget = bShowDragGhost && DragGhostInstanceId == Resolved[Index].InstanceId;
		const FRotator ModuleYaw(0.0f, static_cast<float>(Resolved[Index].YawStep) * 90.0f, 0.0f);
		const FVector Center = bIsDragGhostTarget ? GetDragGhostWorldCenter(Size) : Resolved[Index].Center;
		const FTransform ModuleTransform(ModuleYaw, Center, FVector::OneVector);

		// Ручной override визуала полностью заменяет процедурную генерацию "коробки".
		if (const UShipModuleVisualOverride* VisualOverride = Def->GetVisualOverride())
		{
			if (VisualOverride->VisualParts.Num() > 0)
			{
				for (const FShipModuleVisualPart& Part : VisualOverride->VisualParts)
				{
					UStaticMesh* PartMesh = Part.Mesh.Get();
					if (!PartMesh)
					{
						continue;
					}

					UInstancedStaticMeshComponent& OverridePool = GetOrCreatePool(OverrideMeshPools, PartMesh, TEXT("Override"));
					FTransform FinalTransform = Part.RelativeTransform * ModuleTransform;
					AddTransformInstance(OverridePool, FinalTransform);
				}

				// Для override-модулей тоже показываем hover/selection и сокеты.
				if (bIsSelected || bIsHovered || (bShowDragGhost && DragGhostInstanceId == Resolved[Index].InstanceId))
				{
					UInstancedStaticMeshComponent& SelectionPoolOverride = GetOrCreatePool(SelectionMeshPools, PanelMesh, TEXT("SelectionOverride"));
					UInstancedStaticMeshComponent& SocketPoolOverride = GetOrCreatePool(SocketMarkerPools, PanelMesh, TEXT("SocketMarkerOverride"));
					ConfigureSelectionComponent(SelectionPoolOverride);
					ConfigureSocketMarkerComponent(SocketPoolOverride);

					const float Pad = FMath::Max(0.0f, bIsHovered ? SelectionOutlinePadding * 0.6f : SelectionOutlinePadding);
					const float Thickness = FMath::Clamp(SelectionOutlineThickness, 1.0f, 40.0f);
					if (bIsSelected || bIsHovered)
					{
						AddRotatedSelectionOutline(SelectionPoolOverride, ModuleYaw, Center, Size, Pad, Thickness);
					}
					if (bShowDragGhost && DragGhostInstanceId == Resolved[Index].InstanceId)
					{
						const FVector GhostCenter = GetDragGhostWorldCenter(Size);
						AddRotatedSelectionOutline(SelectionPoolOverride, ModuleYaw, GhostCenter, Size, Pad, Thickness);
					}

					if (bIsHovered || bIsSelected)
					{
						const float Marker = FMath::Clamp(SocketMarkerSize, 8.0f, 64.0f);
						AddEffectiveSocketMarkerInstances(
							*Def,
							ModuleTransform,
							SocketPoolOverride,
							Marker,
							SocketMarkerOffset);
					}
				}
				continue;
			}
		}

		if (!Def->bHasInterior)
		{
			UStaticMesh* SolidMesh = ResolveMeshForType(
				ModuleTypeSolidMeshOverrides,
				SolidModuleMesh ? SolidModuleMesh : PanelMesh,
				Def->ModuleType);
			if (SolidMesh)
			{
				UInstancedStaticMeshComponent& SolidPool = GetOrCreatePool(SolidMeshPools, SolidMesh, TEXT("Solid"));
				AddTransformInstance(SolidPool, FTransform(ModuleYaw, Center, Size / 100.0f));
			}

			if (bIsSelected || bIsHovered || (bShowDragGhost && DragGhostInstanceId == Resolved[Index].InstanceId))
			{
				UInstancedStaticMeshComponent& SelectionPoolSolid = GetOrCreatePool(SelectionMeshPools, PanelMesh, TEXT("SelectionSolid"));
				UInstancedStaticMeshComponent& SocketPoolSolid = GetOrCreatePool(SocketMarkerPools, PanelMesh, TEXT("SocketMarkerSolid"));
				if (bIsSelected || bIsHovered)
				{
					ConfigureSelectionComponent(SelectionPoolSolid);
					ConfigureSocketMarkerComponent(SocketPoolSolid);
					const float Pad = FMath::Max(0.0f, bIsHovered ? SelectionOutlinePadding * 0.6f : SelectionOutlinePadding);
					const float Thickness = FMath::Clamp(SelectionOutlineThickness, 1.0f, 40.0f);
					AddRotatedSelectionOutline(SelectionPoolSolid, ModuleYaw, Center, Size, Pad, Thickness);
					const float Marker = FMath::Clamp(SocketMarkerSize, 8.0f, 64.0f);
					AddEffectiveSocketMarkerInstances(*Def, ModuleTransform, SocketPoolSolid, Marker, SocketMarkerOffset);
				}
				if (bShowDragGhost && DragGhostInstanceId == Resolved[Index].InstanceId)
				{
					ConfigureSelectionComponent(SelectionPoolSolid);
					const float Pad = FMath::Max(0.0f, SelectionOutlinePadding);
					const float Thickness = FMath::Clamp(SelectionOutlineThickness, 1.0f, 40.0f);
					const FVector GhostCenter = GetDragGhostWorldCenter(Size);
					AddRotatedSelectionOutline(SelectionPoolSolid, ModuleYaw, GhostCenter, Size, Pad, Thickness);
				}
			}
			continue;
		}

		const FName CurrentInstanceId = Resolved[Index].InstanceId;
		const FIntVector Cells = Def->GetEffectiveCellSize();
		const FVector Half = Size * 0.5f;
		const float PanelXY = ShipBuilderGrid::PanelUnitXY;
		const float PanelZ = ShipBuilderGrid::PanelUnitZ;

		TSet<FName> ConnectedHorizontalDoorSockets;
		TSet<FName> ConnectedVerticalDoorSockets;
		const FShipBuilderPlacedModule* CurrentPlaced = Draft.PlacedModules.FindByPredicate(
			[CurrentInstanceId](const FShipBuilderPlacedModule& Placed)
			{
				return Placed.InstanceId == CurrentInstanceId;
			});
		const FShipBuilderModuleWorldPlacement CurrentPlacement = CurrentPlaced
			? SpaceshipCrew_BuildModuleWorldPlacement(
				*CurrentPlaced,
				*Def,
				ShipBuilderPreviewActorPrivate::GridStepXY,
				ShipBuilderPreviewActorPrivate::GridStepZ)
			: FShipBuilderModuleWorldPlacement();

		auto AddResolvedConnectionSocket = [&](const FName ConnectionSocketName)
		{
			const FName PanelSocket = CurrentPlaced
				? SpaceshipCrew_ResolvePanelSocketName(
					*Def,
					ConnectionSocketName,
					&CurrentPlacement,
					Resolved[Index].YawStep)
				: ConnectionSocketName;
			if (ShipBuilderPreviewActorPrivate::IsHorizontalDoorwaySocket(*Def, PanelSocket)
				|| ShipBuilderPreviewActorPrivate::IsHorizontalDoorwaySocket(*Def, ConnectionSocketName))
			{
				ConnectedHorizontalDoorSockets.Add(PanelSocket);
				ConnectedHorizontalDoorSockets.Add(ConnectionSocketName);
			}
			else if (ShipBuilderPreviewActorPrivate::IsVerticalDoorwaySocket(*Def, PanelSocket)
				|| ShipBuilderPreviewActorPrivate::IsVerticalDoorwaySocket(*Def, ConnectionSocketName))
			{
				ConnectedVerticalDoorSockets.Add(PanelSocket);
				ConnectedVerticalDoorSockets.Add(ConnectionSocketName);
			}
		};

		auto ShouldCreateOpeningTowardNeighbor = [&](const FName NeighborInstanceId) -> bool
		{
			const FShipBuilderPlacedModule* NeighborPlaced = Draft.PlacedModules.FindByPredicate(
				[NeighborInstanceId](const FShipBuilderPlacedModule& Placed)
				{
					return Placed.InstanceId == NeighborInstanceId;
				});
			if (!NeighborPlaced)
			{
				return true;
			}
			const UShipModuleDefinition* NeighborDef = Catalog.FindModuleById(NeighborPlaced->ModuleId);
			return !NeighborDef || NeighborDef->bHasInterior;
		};

		for (const FShipBuilderDraftConnection& Connection : Draft.Connections)
		{
			const bool bCurrentAsA = Connection.ModuleAInstanceId == CurrentInstanceId;
			const bool bCurrentAsB = Connection.ModuleBInstanceId == CurrentInstanceId;
			if (!bCurrentAsA && !bCurrentAsB)
			{
				continue;
			}
			const FName NeighborInstanceId = bCurrentAsA
				? Connection.ModuleBInstanceId
				: Connection.ModuleAInstanceId;
			if (!ShouldCreateOpeningTowardNeighbor(NeighborInstanceId))
			{
				continue;
			}
			AddResolvedConnectionSocket(bCurrentAsA ? Connection.ModuleASocketName : Connection.ModuleBSocketName);
		}

		if (Draft.Connections.Num() == 0 && bHasDomainChain)
		{
			for (const FShipBuildModuleConnection& Connection : DomainModel.GetConnections())
			{
				const bool bCurrentAsA = Connection.ModuleAInstanceId == CurrentInstanceId;
				const bool bCurrentAsB = Connection.ModuleBInstanceId == CurrentInstanceId;
				if (!bCurrentAsA && !bCurrentAsB)
				{
					continue;
				}
				const FName NeighborInstanceId = bCurrentAsA
					? Connection.ModuleBInstanceId
					: Connection.ModuleAInstanceId;
				if (!ShouldCreateOpeningTowardNeighbor(NeighborInstanceId))
				{
					continue;
				}
				AddResolvedConnectionSocket(bCurrentAsA ? Connection.ModuleASocketName : Connection.ModuleBSocketName);
			}
		}

		TSet<FName> ForcedOpeningSockets;
		if (ShouldForceOpeningForSide(*Def, EShipModuleOpeningSide::Front))
		{
			ForcedOpeningSockets.Add(ShipBuilderPreviewActorPrivate::ForcedOpeningSocketForSide(Cells, EShipModuleOpeningSide::Front));
		}
		if (ShouldForceOpeningForSide(*Def, EShipModuleOpeningSide::Back))
		{
			ForcedOpeningSockets.Add(ShipBuilderPreviewActorPrivate::ForcedOpeningSocketForSide(Cells, EShipModuleOpeningSide::Back));
		}
		if (ShouldForceOpeningForSide(*Def, EShipModuleOpeningSide::Left))
		{
			ForcedOpeningSockets.Add(ShipBuilderPreviewActorPrivate::ForcedOpeningSocketForSide(Cells, EShipModuleOpeningSide::Left));
		}
		if (ShouldForceOpeningForSide(*Def, EShipModuleOpeningSide::Right))
		{
			ForcedOpeningSockets.Add(ShipBuilderPreviewActorPrivate::ForcedOpeningSocketForSide(Cells, EShipModuleOpeningSide::Right));
		}

		const float X = Size.X;
		const float Y = Size.Y;
		const float Z = Size.Z;
		const float T = FMath::Clamp(PanelThickness, 2.0f, FMath::Min3(X, Y, Z) * 0.25f);

		UStaticMesh* SelectedPanelMesh = ResolveMeshForType(ModuleTypePanelMeshOverrides, PanelMesh, Def->ModuleType);
		if (!SelectedPanelMesh)
		{
			continue;
		}
		UStaticMesh* SelectedDamagedMesh = DamagedPanelMesh.Get() ? DamagedPanelMesh.Get() : SelectedPanelMesh;
		UStaticMesh* SelectedFrameMesh = DoorFrameMesh.Get() ? DoorFrameMesh.Get() : SelectedPanelMesh;

		UInstancedStaticMeshComponent& PanelPool = GetOrCreatePool(PanelMeshPools, SelectedPanelMesh, TEXT("Panel"));
		UInstancedStaticMeshComponent* DamagedPool = SelectedDamagedMesh
			? &GetOrCreatePool(DamagedPanelMeshPools, SelectedDamagedMesh, TEXT("Damaged"))
			: nullptr;
		UInstancedStaticMeshComponent& FramePool = GetOrCreatePool(DoorFrameMeshPools, SelectedFrameMesh, TEXT("DoorFrame"));
		UInstancedStaticMeshComponent& SelectionPool = GetOrCreatePool(SelectionMeshPools, SelectedFrameMesh, TEXT("Selection"));
		UInstancedStaticMeshComponent& SocketPool = GetOrCreatePool(SocketMarkerPools, SelectedFrameMesh, TEXT("SocketMarker"));

		if (bIsSelected || bIsHovered)
		{
			ConfigureSelectionComponent(SelectionPool);
			const float Pad = FMath::Max(0.0f, bIsHovered ? SelectionOutlinePadding * 0.6f : SelectionOutlinePadding);
			const float Thickness = FMath::Clamp(SelectionOutlineThickness, 1.0f, 40.0f);
			AddRotatedSelectionOutline(SelectionPool, ModuleYaw, Center, Size, Pad, Thickness);
		}

		if (bIsHovered || bIsSelected)
		{
			ConfigureSocketMarkerComponent(SocketPool);
			const float Marker = FMath::Clamp(SocketMarkerSize, 8.0f, 64.0f);
			AddEffectiveSocketMarkerInstances(
				*Def,
				ModuleTransform,
				SocketPool,
				Marker,
				SocketMarkerOffset);
		}

		if (bShowDragGhost && DragGhostInstanceId == Resolved[Index].InstanceId)
		{
			const float Pad = FMath::Max(0.0f, SelectionOutlinePadding);
			const float Thickness = FMath::Clamp(SelectionOutlineThickness, 1.0f, 40.0f);
			const FVector GhostCenter = GetDragGhostWorldCenter(Size);

			ConfigureSelectionComponent(SelectionPool);
			AddRotatedSelectionOutline(SelectionPool, ModuleYaw, GhostCenter, Size, Pad, Thickness);
		}

		auto AddOrientedShellPanel = [&](const FVector& LocalCenter, const FVector& PanelSize)
		{
			const bool bUseDamaged = bPreviewDamage
				&& DamageEveryNthPanel > 1
				&& DamagedPool != nullptr
				&& ((GlobalPanelOrdinal + 1) % DamageEveryNthPanel == 0);
			const FTransform PanelTransform(
				ModuleYaw,
				Center + ModuleYaw.RotateVector(LocalCenter),
				PanelSize / 100.0f);
			AddTransformInstance(bUseDamaged ? *DamagedPool : PanelPool, PanelTransform);
			++GlobalPanelOrdinal;
		};

		auto PanelSolidCenter = [&](const TCHAR* Face, const int32 IX, const int32 IY, const int32 IZ) -> FVector
		{
			const float Px = -Half.X + (static_cast<float>(IX) + 0.5f) * PanelXY;
			const float Py = -Half.Y + (static_cast<float>(IY) + 0.5f) * PanelXY;
			const float Pz = -Half.Z + (static_cast<float>(IZ) + 0.5f) * PanelZ;
			if (FCString::Strcmp(Face, TEXT("Back")) == 0)
			{
				return FVector(-Half.X + T * 0.5f, Py, Pz);
			}
			if (FCString::Strcmp(Face, TEXT("Front")) == 0)
			{
				return FVector(Half.X - T * 0.5f, Py, Pz);
			}
			if (FCString::Strcmp(Face, TEXT("Left")) == 0)
			{
				return FVector(Px, -Half.Y + T * 0.5f, Pz);
			}
			if (FCString::Strcmp(Face, TEXT("Right")) == 0)
			{
				return FVector(Px, Half.Y - T * 0.5f, Pz);
			}
			if (FCString::Strcmp(Face, TEXT("Bottom")) == 0)
			{
				return FVector(Px, Py, -Half.Z + T * 0.5f);
			}
			return FVector(Px, Py, Half.Z - T * 0.5f);
		};

		auto ShouldOpenPanel = [&](const FName PanelSocketName) -> bool
		{
			if (ForcedOpeningSockets.Contains(PanelSocketName))
			{
				return true;
			}
			for (const FName Connected : ConnectedHorizontalDoorSockets)
			{
				if (SpaceshipCrew_DoPanelSocketsMatch(PanelSocketName, Connected))
				{
					return true;
				}
			}
			for (const FName Connected : ConnectedVerticalDoorSockets)
			{
				if (SpaceshipCrew_DoPanelSocketsMatch(PanelSocketName, Connected))
				{
					return true;
				}
			}
			return false;
		};

		auto TryAddVerticalWallPanel = [&](
			const TCHAR* Face,
			const int32 IX,
			const int32 IY,
			const int32 IZ,
			const bool bNormalAlongX)
		{
			const FName SocketName = ShipBuilderPreviewActorPrivate::MakePanelSocketName(Face, IX, IY, IZ);
			if (ShouldOpenPanel(SocketName))
			{
				FVector FrameCenter = PanelSolidCenter(Face, IX, IY, IZ);
				FVector SocketLoc = FVector::ZeroVector;
				if (ShipBuilderPreviewActorPrivate::FindSocketRelativeLocation(*Def, SocketName, SocketLoc))
				{
					FrameCenter = ShipBuilderPreviewActorPrivate::DoorFrameCenterOnWallPlane(SocketLoc, Half, T);
				}
				AddDoorOpeningFrame(FramePool, ModuleYaw, Center, FrameCenter, T, PanelXY, PanelZ, bNormalAlongX);
				return;
			}

			const FVector PanelSize = bNormalAlongX ? FVector(T, PanelXY, PanelZ) : FVector(PanelXY, T, PanelZ);
			AddOrientedShellPanel(PanelSolidCenter(Face, IX, IY, IZ), PanelSize);
		};

		auto TryAddHorizontalDeckPanel = [&](const TCHAR* Face, const int32 IX, const int32 IY, const int32 IZ)
		{
			const FName SocketName = ShipBuilderPreviewActorPrivate::MakePanelSocketName(Face, IX, IY, IZ);
			const FVector PanelCenter = PanelSolidCenter(Face, IX, IY, IZ);
			if (ShouldOpenPanel(SocketName)
				&& ShipBuilderPreviewActorPrivate::IsVerticalDoorwaySocket(*Def, SocketName))
			{
				AddPanelHatchFrame(FramePool, ModuleYaw, Center, PanelCenter, T, PanelXY);
				return;
			}
			AddOrientedShellPanel(PanelCenter, FVector(PanelXY, PanelXY, T));
		};

		for (int32 IX = 0; IX < Cells.X; ++IX)
		{
			for (int32 IY = 0; IY < Cells.Y; ++IY)
			{
				TryAddHorizontalDeckPanel(TEXT("Bottom"), IX, IY, 0);
				TryAddHorizontalDeckPanel(TEXT("Top"), IX, IY, Cells.Z - 1);
			}
		}

		for (int32 IY = 0; IY < Cells.Y; ++IY)
		{
			for (int32 IZ = 0; IZ < Cells.Z; ++IZ)
			{
				TryAddVerticalWallPanel(TEXT("Back"), 0, IY, IZ, true);
				TryAddVerticalWallPanel(TEXT("Front"), Cells.X - 1, IY, IZ, true);
			}
		}

		for (int32 IX = 0; IX < Cells.X; ++IX)
		{
			for (int32 IZ = 0; IZ < Cells.Z; ++IZ)
			{
				TryAddVerticalWallPanel(TEXT("Left"), IX, 0, IZ, false);
				TryAddVerticalWallPanel(TEXT("Right"), IX, Cells.Y - 1, IZ, false);
			}
		}
	}
}

