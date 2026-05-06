#include "ShipBuilder/ShipBuilderModulePreviewActor.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ShipBuilder/ShipBuilderDomainGlue.h"
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

	static void RotateHorizontalOpeningsByYaw(
		const int32 YawStep,
		const bool bFrontIn,
		const bool bBackIn,
		const bool bLeftIn,
		const bool bRightIn,
		bool& bFrontOut,
		bool& bBackOut,
		bool& bLeftOut,
		bool& bRightOut)
	{
		const int32 Step = ((YawStep % 4) + 4) % 4;
		bFrontOut = bFrontIn;
		bBackOut = bBackIn;
		bLeftOut = bLeftIn;
		bRightOut = bRightIn;
		for (int32 i = 0; i < Step; ++i)
		{
			const bool PrevFront = bFrontOut;
			const bool PrevBack = bBackOut;
			const bool PrevLeft = bLeftOut;
			const bool PrevRight = bRightOut;
			// +90 yaw: front->right, right->back, back->left, left->front
			bFrontOut = PrevLeft;
			bRightOut = PrevFront;
			bBackOut = PrevRight;
			bLeftOut = PrevBack;
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

void AShipBuilderModulePreviewActor::AddBoxInstance(
	UInstancedStaticMeshComponent& Component,
	const FVector& Center,
	const FVector& Size) const
{
	const FVector Scale = Size / 100.0f;
	const FTransform Transform(FRotator::ZeroRotator, Center, Scale);
	AddTransformInstance(Component, Transform);
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

void AShipBuilderModulePreviewActor::AddDoorOpeningFrame(
	UInstancedStaticMeshComponent& FrameComponent,
	const FVector& WallCenter,
	const float WallThickness,
	const float WallSpan,
	const float WallHeight,
	const bool bNormalAlongX) const
{
	const float OpenWidth = FMath::Clamp(160.0f, 80.0f, FMath::Max(80.0f, WallSpan - 2.0f * WallThickness));
	const float OpenHeight = FMath::Clamp(220.0f, 120.0f, FMath::Max(120.0f, WallHeight - 2.0f * WallThickness));

	const float SideWidth = (WallSpan - OpenWidth) * 0.5f;
	if (SideWidth > 1.0f)
	{
		if (bNormalAlongX)
		{
			AddBoxInstance(
				FrameComponent,
				WallCenter + FVector(0.0f, -WallSpan * 0.5f + SideWidth * 0.5f, 0.0f),
				FVector(WallThickness, SideWidth, WallHeight));
			AddBoxInstance(
				FrameComponent,
				WallCenter + FVector(0.0f, WallSpan * 0.5f - SideWidth * 0.5f, 0.0f),
				FVector(WallThickness, SideWidth, WallHeight));
		}
		else
		{
			AddBoxInstance(
				FrameComponent,
				WallCenter + FVector(-WallSpan * 0.5f + SideWidth * 0.5f, 0.0f, 0.0f),
				FVector(SideWidth, WallThickness, WallHeight));
			AddBoxInstance(
				FrameComponent,
				WallCenter + FVector(WallSpan * 0.5f - SideWidth * 0.5f, 0.0f, 0.0f),
				FVector(SideWidth, WallThickness, WallHeight));
		}
	}

	const float OpeningTopZ = -WallHeight * 0.5f + WallThickness + OpenHeight;
	const float TopHeight = WallHeight * 0.5f - OpeningTopZ;
	if (TopHeight > 1.0f)
	{
		if (bNormalAlongX)
		{
			AddBoxInstance(
				FrameComponent,
				WallCenter + FVector(0.0f, 0.0f, OpeningTopZ + TopHeight * 0.5f),
				FVector(WallThickness, OpenWidth, TopHeight));
		}
		else
		{
			AddBoxInstance(
				FrameComponent,
				WallCenter + FVector(0.0f, 0.0f, OpeningTopZ + TopHeight * 0.5f),
				FVector(OpenWidth, WallThickness, TopHeight));
		}
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
			Entry.Center = FVector(
				static_cast<float>(Placed.GridPos.X) * ShipBuilderPreviewActorPrivate::GridStepXY,
				static_cast<float>(Placed.GridPos.Y) * ShipBuilderPreviewActorPrivate::GridStepXY,
				static_cast<float>(Placed.GridPos.Z) * ShipBuilderPreviewActorPrivate::GridStepZ + Def->Size.Z * 0.5f);
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

		const FVector Center = Resolved[Index].Center;
		const FVector Size = Def->Size.ComponentMax(FVector(20.0f, 20.0f, 20.0f));
		const bool bIsSelected = !SelectedModuleInstanceId.IsNone()
			&& SelectedModuleInstanceId == Resolved[Index].InstanceId;
		const bool bIsHovered = !HoveredModuleInstanceId.IsNone()
			&& HoveredModuleInstanceId == Resolved[Index].InstanceId;
		const FRotator ModuleYaw(0.0f, static_cast<float>(Resolved[Index].YawStep) * 90.0f, 0.0f);
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
					const float OutlineTopZ = Center.Z + Size.Z * 0.5f + Thickness * 0.5f + Pad * 0.15f;
					const float OuterX = Size.X + 2.0f * Pad;
					const float OuterY = Size.Y + 2.0f * Pad;
					if (bIsSelected || bIsHovered)
					{
						AddBoxInstance(SelectionPoolOverride, FVector(Center.X, Center.Y - OuterY * 0.5f + Thickness * 0.5f, OutlineTopZ), FVector(OuterX, Thickness, Thickness));
						AddBoxInstance(SelectionPoolOverride, FVector(Center.X, Center.Y + OuterY * 0.5f - Thickness * 0.5f, OutlineTopZ), FVector(OuterX, Thickness, Thickness));
						AddBoxInstance(SelectionPoolOverride, FVector(Center.X - OuterX * 0.5f + Thickness * 0.5f, Center.Y, OutlineTopZ), FVector(Thickness, OuterY, Thickness));
						AddBoxInstance(SelectionPoolOverride, FVector(Center.X + OuterX * 0.5f - Thickness * 0.5f, Center.Y, OutlineTopZ), FVector(Thickness, OuterY, Thickness));
					}
					if (bShowDragGhost && DragGhostInstanceId == Resolved[Index].InstanceId)
					{
						const FVector GhostCenter(
							static_cast<float>(DragGhostGridPos.X) * ShipBuilderPreviewActorPrivate::GridStepXY,
							static_cast<float>(DragGhostGridPos.Y) * ShipBuilderPreviewActorPrivate::GridStepXY,
							static_cast<float>(DragGhostGridPos.Z) * ShipBuilderPreviewActorPrivate::GridStepZ + Size.Z * 0.5f);
						const float GhostTopZ = GhostCenter.Z + Size.Z * 0.5f + Thickness * 0.5f + Pad * 0.15f;
						AddBoxInstance(SelectionPoolOverride, FVector(GhostCenter.X, GhostCenter.Y - OuterY * 0.5f + Thickness * 0.5f, GhostTopZ), FVector(OuterX, Thickness, Thickness));
						AddBoxInstance(SelectionPoolOverride, FVector(GhostCenter.X, GhostCenter.Y + OuterY * 0.5f - Thickness * 0.5f, GhostTopZ), FVector(OuterX, Thickness, Thickness));
						AddBoxInstance(SelectionPoolOverride, FVector(GhostCenter.X - OuterX * 0.5f + Thickness * 0.5f, GhostCenter.Y, GhostTopZ), FVector(Thickness, OuterY, Thickness));
						AddBoxInstance(SelectionPoolOverride, FVector(GhostCenter.X + OuterX * 0.5f - Thickness * 0.5f, GhostCenter.Y, GhostTopZ), FVector(Thickness, OuterY, Thickness));
					}

					if (bIsHovered)
					{
						const float Marker = FMath::Clamp(SocketMarkerSize, 8.0f, 64.0f);
						const TArray<FShipModuleContactPoint>& Sockets = Def->GetResolvedContactPoints();
						if (Sockets.Num() > 0)
						{
							for (const FShipModuleContactPoint& CP : Sockets)
							{
								const FVector N = CP.RelativeLocation.GetSafeNormal();
								const FVector Pos = ModuleTransform.TransformPosition(CP.RelativeLocation + N * SocketMarkerOffset);
								AddBoxInstance(SocketPoolOverride, Pos, FVector(Marker, Marker, Marker));
							}
						}
						else
						{
							const FVector Half = Size * 0.5f;
							AddBoxInstance(SocketPoolOverride, ModuleTransform.TransformPosition(FVector(Half.X, 0.0f, 0.0f)), FVector(Marker, Marker, Marker));
							AddBoxInstance(SocketPoolOverride, ModuleTransform.TransformPosition(FVector(-Half.X, 0.0f, 0.0f)), FVector(Marker, Marker, Marker));
							AddBoxInstance(SocketPoolOverride, ModuleTransform.TransformPosition(FVector(0.0f, Half.Y, 0.0f)), FVector(Marker, Marker, Marker));
							AddBoxInstance(SocketPoolOverride, ModuleTransform.TransformPosition(FVector(0.0f, -Half.Y, 0.0f)), FVector(Marker, Marker, Marker));
							AddBoxInstance(SocketPoolOverride, ModuleTransform.TransformPosition(FVector(0.0f, 0.0f, Half.Z)), FVector(Marker, Marker, Marker));
							AddBoxInstance(SocketPoolOverride, ModuleTransform.TransformPosition(FVector(0.0f, 0.0f, -Half.Z)), FVector(Marker, Marker, Marker));
						}
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
			continue;
		}

		const FName CurrentInstanceId = Resolved[Index].InstanceId;
		bool bOpenBackBySocket = false;
		bool bOpenFrontBySocket = false;
		bool bOpenLeftBySocket = false;
		bool bOpenRightBySocket = false;
		bool bHasVerticalConnection = false;
		if (bHasDomainChain)
		{
			auto IsDoorwaySocket = [Def](const FName SocketName) -> bool
			{
				for (const FShipModuleContactPoint& CP : Def->GetResolvedContactPoints())
				{
					if (CP.SocketName == SocketName)
					{
						return CP.SocketType == EShipModuleSocketType::Horizontal
							|| CP.SocketType == EShipModuleSocketType::Universal;
					}
				}
				const FString Socket = SocketName.ToString().ToLower();
				return Socket == TEXT("front")
					|| Socket == TEXT("back")
					|| Socket == TEXT("left")
					|| Socket == TEXT("right");
			};

			for (const FShipBuildModuleConnection& Connection : DomainModel.GetConnections())
			{
				const bool bCurrentAsA = Connection.ModuleAInstanceId == CurrentInstanceId;
				const bool bCurrentAsB = Connection.ModuleBInstanceId == CurrentInstanceId;
				if (!bCurrentAsA && !bCurrentAsB)
				{
					continue;
				}

				const FName OtherInstanceId = bCurrentAsA ? Connection.ModuleBInstanceId : Connection.ModuleAInstanceId;
				const FName CurrentSocketName = bCurrentAsA ? Connection.ModuleASocketName : Connection.ModuleBSocketName;
				const int32* OtherIndexPtr = IndexByInstanceId.Find(OtherInstanceId);
				if (!OtherIndexPtr)
				{
					continue;
				}
				const int32 OtherIndex = *OtherIndexPtr;
				if (!Resolved.IsValidIndex(OtherIndex) || !Resolved[OtherIndex].Def)
				{
					continue;
				}

				const UShipModuleDefinition* OtherDef = Resolved[OtherIndex].Def;
				const FName OtherSocketName = bCurrentAsA ? Connection.ModuleBSocketName : Connection.ModuleASocketName;
				const bool bDoorwayPair = IsDoorwaySocket(CurrentSocketName)
					&& [OtherDef, OtherSocketName]()
					{
						for (const FShipModuleContactPoint& CP : OtherDef->GetResolvedContactPoints())
						{
							if (CP.SocketName == OtherSocketName)
							{
								return CP.SocketType == EShipModuleSocketType::Horizontal
									|| CP.SocketType == EShipModuleSocketType::Universal;
							}
						}
						const FString Socket = OtherSocketName.ToString().ToLower();
						return Socket == TEXT("front")
							|| Socket == TEXT("back")
							|| Socket == TEXT("left")
							|| Socket == TEXT("right");
					}();
				if (!bDoorwayPair)
				{
					continue;
				}

				if (OtherIndex < Index)
				{
					const FVector Delta = Resolved[OtherIndex].Center - Center;
					if (FMath::Abs(Delta.Y) > FMath::Abs(Delta.X))
					{
						if (Delta.Y > 0.0f) bOpenRightBySocket = true;
						else bOpenLeftBySocket = true;
					}
					else
					{
						if (Delta.X > 0.0f) bOpenFrontBySocket = true;
						else bOpenBackBySocket = true;
					}
				}
				else if (OtherIndex > Index)
				{
					const FVector Delta = Resolved[OtherIndex].Center - Center;
					if (FMath::Abs(Delta.Y) > FMath::Abs(Delta.X))
					{
						if (Delta.Y > 0.0f) bOpenRightBySocket = true;
						else bOpenLeftBySocket = true;
					}
					else
					{
						if (Delta.X > 0.0f) bOpenFrontBySocket = true;
						else bOpenBackBySocket = true;
					}
				}
				const bool bVerticalPair = CurrentSocketName.ToString().Contains(TEXT("Top"))
					|| CurrentSocketName.ToString().Contains(TEXT("Bottom"))
					|| OtherSocketName.ToString().Contains(TEXT("Top"))
					|| OtherSocketName.ToString().Contains(TEXT("Bottom"));
				if (bVerticalPair)
				{
					bHasVerticalConnection = true;
				}
			}
		}

		const bool bForceFrontOpening = ShouldForceOpeningForSide(*Def, EShipModuleOpeningSide::Front);
		const bool bForceBackOpening = ShouldForceOpeningForSide(*Def, EShipModuleOpeningSide::Back);
		const bool bForceLeftOpening = ShouldForceOpeningForSide(*Def, EShipModuleOpeningSide::Left);
		const bool bForceRightOpening = ShouldForceOpeningForSide(*Def, EShipModuleOpeningSide::Right);
		bool bWorldOpenFront = false;
		bool bWorldOpenBack = false;
		bool bWorldOpenLeft = false;
		bool bWorldOpenRight = false;
		ShipBuilderPreviewActorPrivate::RotateHorizontalOpeningsByYaw(
			Resolved[Index].YawStep,
			bOpenFrontBySocket || bForceFrontOpening,
			bOpenBackBySocket || bForceBackOpening,
			bOpenLeftBySocket || bForceLeftOpening,
			bOpenRightBySocket || bForceRightOpening,
			bWorldOpenFront,
			bWorldOpenBack,
			bWorldOpenLeft,
			bWorldOpenRight);

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
			const float OutlineTopZ = Center.Z + Size.Z * 0.5f + Thickness * 0.5f + Pad * 0.15f;
			const float OuterX = Size.X + 2.0f * Pad;
			const float OuterY = Size.Y + 2.0f * Pad;

			// Perimeter outline: four thin bars around module footprint.
			AddBoxInstance(
				SelectionPool,
				FVector(Center.X, Center.Y - OuterY * 0.5f + Thickness * 0.5f, OutlineTopZ),
				FVector(OuterX, Thickness, Thickness));
			AddBoxInstance(
				SelectionPool,
				FVector(Center.X, Center.Y + OuterY * 0.5f - Thickness * 0.5f, OutlineTopZ),
				FVector(OuterX, Thickness, Thickness));
			AddBoxInstance(
				SelectionPool,
				FVector(Center.X - OuterX * 0.5f + Thickness * 0.5f, Center.Y, OutlineTopZ),
				FVector(Thickness, OuterY, Thickness));
			AddBoxInstance(
				SelectionPool,
				FVector(Center.X + OuterX * 0.5f - Thickness * 0.5f, Center.Y, OutlineTopZ),
				FVector(Thickness, OuterY, Thickness));
		}

		if (bIsHovered)
		{
			ConfigureSocketMarkerComponent(SocketPool);
			const float Marker = FMath::Clamp(SocketMarkerSize, 8.0f, 64.0f);
			const TArray<FShipModuleContactPoint>& Sockets = Def->GetResolvedContactPoints();
			if (Sockets.Num() > 0)
			{
				for (const FShipModuleContactPoint& CP : Sockets)
				{
					const FVector N = CP.RelativeLocation.GetSafeNormal();
					const FVector Pos = ModuleTransform.TransformPosition(CP.RelativeLocation + N * SocketMarkerOffset);
					AddBoxInstance(SocketPool, Pos, FVector(Marker, Marker, Marker));
				}
			}
			else
			{
				const FVector Half = Size * 0.5f;
				AddBoxInstance(SocketPool, ModuleTransform.TransformPosition(FVector(Half.X, 0.0f, 0.0f)), FVector(Marker, Marker, Marker));
				AddBoxInstance(SocketPool, ModuleTransform.TransformPosition(FVector(-Half.X, 0.0f, 0.0f)), FVector(Marker, Marker, Marker));
				AddBoxInstance(SocketPool, ModuleTransform.TransformPosition(FVector(0.0f, Half.Y, 0.0f)), FVector(Marker, Marker, Marker));
				AddBoxInstance(SocketPool, ModuleTransform.TransformPosition(FVector(0.0f, -Half.Y, 0.0f)), FVector(Marker, Marker, Marker));
				AddBoxInstance(SocketPool, ModuleTransform.TransformPosition(FVector(0.0f, 0.0f, Half.Z)), FVector(Marker, Marker, Marker));
				AddBoxInstance(SocketPool, ModuleTransform.TransformPosition(FVector(0.0f, 0.0f, -Half.Z)), FVector(Marker, Marker, Marker));
			}
		}

		if (bShowDragGhost && DragGhostInstanceId == Resolved[Index].InstanceId)
		{
			const float Pad = FMath::Max(0.0f, SelectionOutlinePadding);
			const float Thickness = FMath::Clamp(SelectionOutlineThickness, 1.0f, 40.0f);
			const FVector GhostCenter(
				static_cast<float>(DragGhostGridPos.X) * ShipBuilderPreviewActorPrivate::GridStepXY,
				static_cast<float>(DragGhostGridPos.Y) * ShipBuilderPreviewActorPrivate::GridStepXY,
				static_cast<float>(DragGhostGridPos.Z) * ShipBuilderPreviewActorPrivate::GridStepZ + Size.Z * 0.5f);
			const float OutlineTopZ = GhostCenter.Z + Size.Z * 0.5f + Thickness * 0.5f + Pad * 0.15f;
			const float OuterX = Size.X + 2.0f * Pad;
			const float OuterY = Size.Y + 2.0f * Pad;

			ConfigureSelectionComponent(SelectionPool);
			AddBoxInstance(
				SelectionPool,
				FVector(GhostCenter.X, GhostCenter.Y - OuterY * 0.5f + Thickness * 0.5f, OutlineTopZ),
				FVector(OuterX, Thickness, Thickness));
			AddBoxInstance(
				SelectionPool,
				FVector(GhostCenter.X, GhostCenter.Y + OuterY * 0.5f - Thickness * 0.5f, OutlineTopZ),
				FVector(OuterX, Thickness, Thickness));
			AddBoxInstance(
				SelectionPool,
				FVector(GhostCenter.X - OuterX * 0.5f + Thickness * 0.5f, GhostCenter.Y, OutlineTopZ),
				FVector(Thickness, OuterY, Thickness));
			AddBoxInstance(
				SelectionPool,
				FVector(GhostCenter.X + OuterX * 0.5f - Thickness * 0.5f, GhostCenter.Y, OutlineTopZ),
				FVector(Thickness, OuterY, Thickness));
		}

		if (bIsHovered)
		{
			TArray<FShipModuleContactPoint> HoverSockets;
			const TArray<FShipModuleContactPoint>& ResolvedSockets = Def->GetResolvedContactPoints();
			if (ResolvedSockets.Num() > 0)
			{
				HoverSockets = ResolvedSockets;
			}
			else
			{
				const FVector Half = Size * 0.5f;
				auto AddDefaultSocket = [&HoverSockets](const TCHAR* Name, const FVector& Loc, const EShipModuleSocketType Type)
				{
					FShipModuleContactPoint CP;
					CP.SocketName = Name;
					CP.RelativeLocation = Loc;
					CP.SocketType = Type;
					HoverSockets.Add(CP);
				};
				AddDefaultSocket(TEXT("Front"), FVector(Half.X, 0.0f, 0.0f), EShipModuleSocketType::Horizontal);
				AddDefaultSocket(TEXT("Back"), FVector(-Half.X, 0.0f, 0.0f), EShipModuleSocketType::Horizontal);
				AddDefaultSocket(TEXT("Left"), FVector(0.0f, -Half.Y, 0.0f), EShipModuleSocketType::Horizontal);
				AddDefaultSocket(TEXT("Right"), FVector(0.0f, Half.Y, 0.0f), EShipModuleSocketType::Horizontal);
				AddDefaultSocket(TEXT("Top"), FVector(0.0f, 0.0f, Half.Z), EShipModuleSocketType::Vertical);
				AddDefaultSocket(TEXT("Bottom"), FVector(0.0f, 0.0f, -Half.Z), EShipModuleSocketType::Vertical);
			}
			ConfigureSelectionComponent(SocketPool);
			const float Marker = FMath::Clamp(SocketMarkerSize, 8.0f, 64.0f);
			for (const FShipModuleContactPoint& CP : HoverSockets)
			{
				const FVector WorldPos = ModuleTransform.TransformPosition(CP.RelativeLocation);
				AddBoxInstance(SocketPool, WorldPos, FVector(Marker, Marker, Marker));
			}
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

		// Пол и потолок.
		AddOrientedShellPanel(FVector(0.0f, 0.0f, -Z * 0.5f + T * 0.5f), FVector(X, Y, T));
		AddOrientedShellPanel(FVector(0.0f, 0.0f, Z * 0.5f - T * 0.5f), FVector(X, Y, T));
		if (bHasVerticalConnection)
		{
			const float RampWidth = FMath::Clamp(Y * 0.32f, 70.0f, 140.0f);
			const float RampLength = FMath::Clamp(X * 0.70f, 180.0f, X - 2.0f * T);
			const float RampRise = FMath::Clamp(Z * 0.45f, 80.0f, Z * 0.6f);
			FTransform RampTransform(
				FRotator(-25.0f, static_cast<float>(Resolved[Index].YawStep) * 90.0f, 0.0f),
				Center + FVector(-X * 0.12f, 0.0f, -Z * 0.5f + T + RampRise * 0.5f),
				FVector(RampLength / 100.0f, RampWidth / 100.0f, T / 100.0f));
			AddTransformInstance(PanelPool, RampTransform);
		}

		// Боковые стенки.
		if (bWorldOpenLeft)
		{
			AddDoorOpeningFrame(FramePool, Center + ModuleYaw.RotateVector(FVector(0.0f, -Y * 0.5f + T * 0.5f, 0.0f)), T, X, Z, false);
		}
		else
		{
			AddOrientedShellPanel(FVector(0.0f, -Y * 0.5f + T * 0.5f, 0.0f), FVector(X, T, Z));
		}
		if (bWorldOpenRight)
		{
			AddDoorOpeningFrame(FramePool, Center + ModuleYaw.RotateVector(FVector(0.0f, Y * 0.5f - T * 0.5f, 0.0f)), T, X, Z, false);
		}
		else
		{
			AddOrientedShellPanel(FVector(0.0f, Y * 0.5f - T * 0.5f, 0.0f), FVector(X, T, Z));
		}

		// Тыльная и фронтальная стенки. На стыке interior-модулей ставим рамку дверного проёма.
		if (!bWorldOpenBack)
		{
			AddOrientedShellPanel(FVector(-X * 0.5f + T * 0.5f, 0.0f, 0.0f), FVector(T, Y, Z));
		}
		else
		{
			AddDoorOpeningFrame(FramePool, Center + ModuleYaw.RotateVector(FVector(-X * 0.5f + T * 0.5f, 0.0f, 0.0f)), T, Y, Z, true);
		}

		if (!bWorldOpenFront)
		{
			AddOrientedShellPanel(FVector(X * 0.5f - T * 0.5f, 0.0f, 0.0f), FVector(T, Y, Z));
		}
		else
		{
			AddDoorOpeningFrame(FramePool, Center + ModuleYaw.RotateVector(FVector(X * 0.5f - T * 0.5f, 0.0f, 0.0f)), T, Y, Z, true);
		}
	}
}

