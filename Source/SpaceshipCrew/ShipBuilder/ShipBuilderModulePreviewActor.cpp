#include "ShipBuilder/ShipBuilderModulePreviewActor.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "ShipModule/ShipModuleCatalog.h"
#include "ShipModule/ShipModuleDefinition.h"
#include "ShipModule/ShipModuleTypes.h"
#include "UObject/ConstructorHelpers.h"

AShipBuilderModulePreviewActor::AShipBuilderModulePreviewActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Static);
	SetRootComponent(Root);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		PanelMesh = CubeMesh.Object;
		DoorFrameMesh = CubeMesh.Object;
		DamagedPanelMesh = CubeMesh.Object;
		SolidModuleMesh = CubeMesh.Object;
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

void AShipBuilderModulePreviewActor::AddBoxInstance(
	UInstancedStaticMeshComponent& Component,
	const FVector& Center,
	const FVector& Size) const
{
	const FVector Scale = Size / 100.0f;
	const FTransform Transform(FRotator::ZeroRotator, Center, Scale);
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
	};

	TArray<FResolvedModule> Resolved;
	Resolved.Reserve(Draft.ModuleIds.Num());

	float CursorX = 0.0f;
	for (const FName ModuleId : Draft.ModuleIds)
	{
		const UShipModuleDefinition* Def = Catalog.FindModuleById(ModuleId);
		if (!Def)
		{
			continue;
		}

		FResolvedModule Entry;
		Entry.Def = Def;
		Entry.Center = FVector(CursorX + Def->Size.X * 0.5f, 0.0f, Def->Size.Z * 0.5f);
		Resolved.Add(Entry);

		CursorX += Def->Size.X + ModuleGap;
	}

	int32 GlobalPanelOrdinal = 0;
	for (int32 Index = 0; Index < Resolved.Num(); ++Index)
	{
		const UShipModuleDefinition* Def = Resolved[Index].Def;
		if (!Def)
		{
			continue;
		}

		const FVector Center = Resolved[Index].Center;
		const FVector Size = Def->Size.ComponentMax(FVector(20.0f, 20.0f, 20.0f));
		if (!Def->bHasInterior)
		{
			UStaticMesh* SolidMesh = ResolveMeshForType(
				ModuleTypeSolidMeshOverrides,
				SolidModuleMesh ? SolidModuleMesh : PanelMesh,
				Def->ModuleType);
			if (SolidMesh)
			{
				UInstancedStaticMeshComponent& SolidPool = GetOrCreatePool(SolidMeshPools, SolidMesh, TEXT("Solid"));
				AddBoxInstance(SolidPool, Center, Size);
			}
			continue;
		}

		const bool bHasPrevInterior = Index > 0 && Resolved[Index - 1].Def && Resolved[Index - 1].Def->bHasInterior;
		const bool bHasNextInterior = Index + 1 < Resolved.Num() && Resolved[Index + 1].Def && Resolved[Index + 1].Def->bHasInterior;
		const bool bForceFrontOpening = ShouldForceOpeningForSide(*Def, EShipModuleOpeningSide::Front);
		const bool bForceBackOpening = ShouldForceOpeningForSide(*Def, EShipModuleOpeningSide::Back);
		const bool bForceLeftOpening = ShouldForceOpeningForSide(*Def, EShipModuleOpeningSide::Left);
		const bool bForceRightOpening = ShouldForceOpeningForSide(*Def, EShipModuleOpeningSide::Right);

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

		// Пол и потолок.
		AddShellPanel(PanelPool, DamagedPool, Center + FVector(0.0f, 0.0f, -Z * 0.5f + T * 0.5f), FVector(X, Y, T), GlobalPanelOrdinal);
		AddShellPanel(PanelPool, DamagedPool, Center + FVector(0.0f, 0.0f, Z * 0.5f - T * 0.5f), FVector(X, Y, T), GlobalPanelOrdinal);

		// Боковые стенки.
		if (bForceLeftOpening)
		{
			AddDoorOpeningFrame(FramePool, Center + FVector(0.0f, -Y * 0.5f + T * 0.5f, 0.0f), T, X, Z, false);
		}
		else
		{
			AddShellPanel(PanelPool, DamagedPool, Center + FVector(0.0f, -Y * 0.5f + T * 0.5f, 0.0f), FVector(X, T, Z), GlobalPanelOrdinal);
		}
		if (bForceRightOpening)
		{
			AddDoorOpeningFrame(FramePool, Center + FVector(0.0f, Y * 0.5f - T * 0.5f, 0.0f), T, X, Z, false);
		}
		else
		{
			AddShellPanel(PanelPool, DamagedPool, Center + FVector(0.0f, Y * 0.5f - T * 0.5f, 0.0f), FVector(X, T, Z), GlobalPanelOrdinal);
		}

		// Тыльная и фронтальная стенки. На стыке interior-модулей ставим рамку дверного проёма.
		if (!bHasPrevInterior && !bForceBackOpening)
		{
			AddShellPanel(PanelPool, DamagedPool, Center + FVector(-X * 0.5f + T * 0.5f, 0.0f, 0.0f), FVector(T, Y, Z), GlobalPanelOrdinal);
		}
		else
		{
			AddDoorOpeningFrame(FramePool, Center + FVector(-X * 0.5f + T * 0.5f, 0.0f, 0.0f), T, Y, Z, true);
		}

		if (!bHasNextInterior && !bForceFrontOpening)
		{
			AddShellPanel(PanelPool, DamagedPool, Center + FVector(X * 0.5f - T * 0.5f, 0.0f, 0.0f), FVector(T, Y, Z), GlobalPanelOrdinal);
		}
		else
		{
			AddDoorOpeningFrame(FramePool, Center + FVector(X * 0.5f - T * 0.5f, 0.0f, 0.0f), T, Y, Z, true);
		}
	}
}

