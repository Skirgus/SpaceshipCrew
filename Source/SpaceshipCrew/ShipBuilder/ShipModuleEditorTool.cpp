#include "ShipBuilder/ShipModuleEditorTool.h"

#include "Engine/StaticMesh.h"
#include "ShipModule/ShipModuleDefinition.h"
#include "ShipModule/ShipModuleVisualOverride.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"
#endif

#if WITH_EDITOR

void UShipModuleEditorTool::GenerateOrUpdateVisualOverride()
{
	if (!TargetModuleDefinition)
	{
		UE_LOG(LogTemp, Warning, TEXT("ShipModuleEditorTool: TargetModuleDefinition не назначен."));
		return;
	}

	UShipModuleVisualOverride* OverrideAsset = ResolveOrCreateOverrideAsset();
	if (!OverrideAsset)
	{
		UE_LOG(LogTemp, Warning, TEXT("ShipModuleEditorTool: не удалось получить или создать VisualOverride."));
		return;
	}

	if (bOverwriteExistingVisualParts)
	{
		OverrideAsset->VisualParts.Reset();
	}
	FillVisualPartsFromDefinition(*OverrideAsset, *TargetModuleDefinition);

	OverrideAsset->MarkPackageDirty();
	if (bAutoAssignOverrideToDefinition)
	{
		TargetModuleDefinition->VisualOverride = OverrideAsset;
		TargetModuleDefinition->MarkPackageDirty();
	}
	MarkPackageDirty();

	UE_LOG(LogTemp, Log, TEXT("ShipModuleEditorTool: VisualOverride обновлён для '%s'."), *TargetModuleDefinition->GetName());
}

void UShipModuleEditorTool::CopyContactPointsToOverride()
{
	if (!TargetModuleDefinition)
	{
		UE_LOG(LogTemp, Warning, TEXT("ShipModuleEditorTool: TargetModuleDefinition не назначен."));
		return;
	}

	UShipModuleVisualOverride* OverrideAsset = ResolveOrCreateOverrideAsset();
	if (!OverrideAsset)
	{
		return;
	}

	OverrideAsset->bOverrideContactPoints = true;
	OverrideAsset->ContactPointsOverride = TargetModuleDefinition->ContactPoints;
	OverrideAsset->MarkPackageDirty();

	if (bAutoAssignOverrideToDefinition)
	{
		TargetModuleDefinition->VisualOverride = OverrideAsset;
		TargetModuleDefinition->MarkPackageDirty();
	}
	MarkPackageDirty();

	UE_LOG(LogTemp, Log, TEXT("ShipModuleEditorTool: ContactPoints скопированы в override '%s'."), *OverrideAsset->GetName());
}

void UShipModuleEditorTool::AddContactPointToOverride()
{
	UShipModuleVisualOverride* OverrideAsset = ResolveOrCreateOverrideAsset();
	if (!OverrideAsset)
	{
		return;
	}

	OverrideAsset->Modify();
	OverrideAsset->bOverrideContactPoints = true;

	FShipModuleContactPoint NewPoint;
	if (TargetModuleDefinition)
	{
		const FVector Size = TargetModuleDefinition->Size.ComponentMax(FVector(20.0f, 20.0f, 20.0f));
		NewPoint.RelativeLocation = FVector(Size.X * 0.5f, 0.0f, 0.0f);
	}
	else
	{
		NewPoint.RelativeLocation = FVector(100.0f, 0.0f, 0.0f);
	}
	NewPoint.RelativeRotation = FRotator::ZeroRotator;
	NewPoint.SocketType = EShipModuleSocketType::Horizontal;

	int32 Suffix = OverrideAsset->ContactPointsOverride.Num();
	FName CandidateName = *FString::Printf(TEXT("Socket_%d"), Suffix);
	while (OverrideAsset->ContactPointsOverride.ContainsByPredicate(
		[CandidateName](const FShipModuleContactPoint& Existing)
		{
			return Existing.SocketName == CandidateName;
		}))
	{
		++Suffix;
		CandidateName = *FString::Printf(TEXT("Socket_%d"), Suffix);
	}
	NewPoint.SocketName = CandidateName;

	OverrideAsset->ContactPointsOverride.Add(NewPoint);
	OverrideAsset->MarkPackageDirty();

	if (bAutoAssignOverrideToDefinition && TargetModuleDefinition)
	{
		TargetModuleDefinition->Modify();
		TargetModuleDefinition->VisualOverride = OverrideAsset;
		TargetModuleDefinition->MarkPackageDirty();
	}

	MarkPackageDirty();
}

void UShipModuleEditorTool::RemoveLastContactPointFromOverride()
{
	UShipModuleVisualOverride* OverrideAsset = ResolveOrCreateOverrideAsset();
	if (!OverrideAsset || OverrideAsset->ContactPointsOverride.Num() == 0)
	{
		return;
	}

	OverrideAsset->Modify();
	OverrideAsset->bOverrideContactPoints = true;
	OverrideAsset->ContactPointsOverride.RemoveAt(OverrideAsset->ContactPointsOverride.Num() - 1);
	OverrideAsset->MarkPackageDirty();

	if (bAutoAssignOverrideToDefinition && TargetModuleDefinition)
	{
		TargetModuleDefinition->Modify();
		TargetModuleDefinition->VisualOverride = OverrideAsset;
		TargetModuleDefinition->MarkPackageDirty();
	}

	MarkPackageDirty();
}

UShipModuleVisualOverride* UShipModuleEditorTool::ResolveOrCreateOverrideAsset()
{
	if (TargetVisualOverride)
	{
		return TargetVisualOverride;
	}
	if (!TargetModuleDefinition)
	{
		return nullptr;
	}

	const FString ModulePackageName = TargetModuleDefinition->GetOutermost()->GetName();
	const FString PackagePath = FPackageName::GetLongPackagePath(ModulePackageName);
	const FString AssetName = TargetModuleDefinition->GetName() + TEXT("_VisualOverride");
	const FString OverridePackageName = PackagePath / AssetName;

	UPackage* Package = CreatePackage(*OverridePackageName);
	if (!Package)
	{
		return nullptr;
	}

	if (UObject* Existing = StaticFindObject(UShipModuleVisualOverride::StaticClass(), Package, *AssetName))
	{
		TargetVisualOverride = Cast<UShipModuleVisualOverride>(Existing);
		return TargetVisualOverride;
	}

	UShipModuleVisualOverride* NewAsset = NewObject<UShipModuleVisualOverride>(
		Package,
		UShipModuleVisualOverride::StaticClass(),
		*AssetName,
		RF_Public | RF_Standalone | RF_Transactional);
	if (!NewAsset)
	{
		return nullptr;
	}

	FAssetRegistryModule::AssetCreated(NewAsset);
	Package->MarkPackageDirty();
	TargetVisualOverride = NewAsset;
	return NewAsset;
}

void UShipModuleEditorTool::AddBoxPart(
	UShipModuleVisualOverride& OverrideAsset,
	UStaticMesh* Mesh,
	const FVector& Center,
	const FVector& Size) const
{
	if (!Mesh)
	{
		return;
	}

	FShipModuleVisualPart Part;
	Part.Mesh = Mesh;
	Part.RelativeTransform = FTransform(FRotator::ZeroRotator, Center, Size / 100.0f);
	OverrideAsset.VisualParts.Add(Part);
}

void UShipModuleEditorTool::AddDoorFrame(
	UShipModuleVisualOverride& OverrideAsset,
	UStaticMesh* Mesh,
	const FVector& WallCenter,
	const float WallThickness,
	const float WallSpan,
	const float WallHeight,
	const bool bNormalAlongX) const
{
	if (!Mesh)
	{
		return;
	}

	const float OpenWidth = FMath::Clamp(160.0f, 80.0f, FMath::Max(80.0f, WallSpan - 2.0f * WallThickness));
	const float OpenHeight = FMath::Clamp(220.0f, 120.0f, FMath::Max(120.0f, WallHeight - 2.0f * WallThickness));
	const float SideWidth = (WallSpan - OpenWidth) * 0.5f;
	if (SideWidth > 1.0f)
	{
		if (bNormalAlongX)
		{
			AddBoxPart(OverrideAsset, Mesh, WallCenter + FVector(0.0f, -WallSpan * 0.5f + SideWidth * 0.5f, 0.0f), FVector(WallThickness, SideWidth, WallHeight));
			AddBoxPart(OverrideAsset, Mesh, WallCenter + FVector(0.0f, WallSpan * 0.5f - SideWidth * 0.5f, 0.0f), FVector(WallThickness, SideWidth, WallHeight));
		}
		else
		{
			AddBoxPart(OverrideAsset, Mesh, WallCenter + FVector(-WallSpan * 0.5f + SideWidth * 0.5f, 0.0f, 0.0f), FVector(SideWidth, WallThickness, WallHeight));
			AddBoxPart(OverrideAsset, Mesh, WallCenter + FVector(WallSpan * 0.5f - SideWidth * 0.5f, 0.0f, 0.0f), FVector(SideWidth, WallThickness, WallHeight));
		}
	}

	const float OpeningTopZ = -WallHeight * 0.5f + WallThickness + OpenHeight;
	const float TopHeight = WallHeight * 0.5f - OpeningTopZ;
	if (TopHeight > 1.0f)
	{
		if (bNormalAlongX)
		{
			AddBoxPart(OverrideAsset, Mesh, WallCenter + FVector(0.0f, 0.0f, OpeningTopZ + TopHeight * 0.5f), FVector(WallThickness, OpenWidth, TopHeight));
		}
		else
		{
			AddBoxPart(OverrideAsset, Mesh, WallCenter + FVector(0.0f, 0.0f, OpeningTopZ + TopHeight * 0.5f), FVector(OpenWidth, WallThickness, TopHeight));
		}
	}
}

bool UShipModuleEditorTool::ShouldForceOpeningForSide(
	const UShipModuleDefinition& Definition,
	const EShipModuleOpeningSide Side)
{
	return Definition.ModuleType == EShipModuleType::Airlock
		&& Definition.ForcedOpeningSide == Side;
}

void UShipModuleEditorTool::FillVisualPartsFromDefinition(
	UShipModuleVisualOverride& OverrideAsset,
	const UShipModuleDefinition& Definition) const
{
	UStaticMesh* Mesh = DefaultPartMesh.Get();
	if (!Mesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("ShipModuleEditorTool: DefaultPartMesh не назначен."));
		return;
	}

	const FVector Size = Definition.Size.ComponentMax(FVector(20.0f, 20.0f, 20.0f));
	if (!Definition.bHasInterior)
	{
		AddBoxPart(OverrideAsset, Mesh, FVector::ZeroVector, Size);
		return;
	}

	const float X = Size.X;
	const float Y = Size.Y;
	const float Z = Size.Z;
	const float T = FMath::Clamp(PanelThickness, 2.0f, FMath::Min3(X, Y, Z) * 0.25f);

	const bool bForceFrontOpening = ShouldForceOpeningForSide(Definition, EShipModuleOpeningSide::Front);
	const bool bForceBackOpening = ShouldForceOpeningForSide(Definition, EShipModuleOpeningSide::Back);
	const bool bForceLeftOpening = ShouldForceOpeningForSide(Definition, EShipModuleOpeningSide::Left);
	const bool bForceRightOpening = ShouldForceOpeningForSide(Definition, EShipModuleOpeningSide::Right);

	AddBoxPart(OverrideAsset, Mesh, FVector(0.0f, 0.0f, -Z * 0.5f + T * 0.5f), FVector(X, Y, T));
	AddBoxPart(OverrideAsset, Mesh, FVector(0.0f, 0.0f, Z * 0.5f - T * 0.5f), FVector(X, Y, T));

	if (bForceLeftOpening)
	{
		AddDoorFrame(OverrideAsset, Mesh, FVector(0.0f, -Y * 0.5f + T * 0.5f, 0.0f), T, X, Z, false);
	}
	else
	{
		AddBoxPart(OverrideAsset, Mesh, FVector(0.0f, -Y * 0.5f + T * 0.5f, 0.0f), FVector(X, T, Z));
	}
	if (bForceRightOpening)
	{
		AddDoorFrame(OverrideAsset, Mesh, FVector(0.0f, Y * 0.5f - T * 0.5f, 0.0f), T, X, Z, false);
	}
	else
	{
		AddBoxPart(OverrideAsset, Mesh, FVector(0.0f, Y * 0.5f - T * 0.5f, 0.0f), FVector(X, T, Z));
	}

	if (bForceBackOpening)
	{
		AddDoorFrame(OverrideAsset, Mesh, FVector(-X * 0.5f + T * 0.5f, 0.0f, 0.0f), T, Y, Z, true);
	}
	else
	{
		AddBoxPart(OverrideAsset, Mesh, FVector(-X * 0.5f + T * 0.5f, 0.0f, 0.0f), FVector(T, Y, Z));
	}

	if (bForceFrontOpening)
	{
		AddDoorFrame(OverrideAsset, Mesh, FVector(X * 0.5f - T * 0.5f, 0.0f, 0.0f), T, Y, Z, true);
	}
	else
	{
		AddBoxPart(OverrideAsset, Mesh, FVector(X * 0.5f - T * 0.5f, 0.0f, 0.0f), FVector(T, Y, Z));
	}
}

#endif

