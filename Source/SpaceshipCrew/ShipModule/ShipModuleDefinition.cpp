#include "ShipModuleDefinition.h"
#include "ShipBuilder/ShipBuilderGridConstants.h"
#include "ShipModuleVisualOverride.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#include "Logging/MessageLog.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogShipModule, Log, All);

// ----------------------------------------------------------------------------
// UPrimaryDataAsset
// ----------------------------------------------------------------------------

FPrimaryAssetId UShipModuleDefinition::GetPrimaryAssetId() const
{
	if (ModuleId.IsNone())
	{
		return Super::GetPrimaryAssetId();
	}
	return FPrimaryAssetId(TEXT("ShipModule"), ModuleId);
}

int32 UShipModuleDefinition::GetEffectiveCreditCost() const
{
	if (CreditCost > 0)
	{
		return CreditCost;
	}
	return FMath::Max(1, FMath::RoundToInt(Mass));
}

FIntVector UShipModuleDefinition::GetEffectiveCellSize() const
{
	return ShipBuilderGrid::GetEffectiveCellSize(CellSize);
}

void UShipModuleDefinition::SyncCellSizeAndSizeFromLegacy()
{
	CellSize = GetEffectiveCellSize();
	Size = ShipBuilderGrid::CellSizeToWorldSize(CellSize);
}

#if WITH_EDITOR

void UShipModuleDefinition::MigrateLegacySizeToCellSizeIfNeeded()
{
	const FIntVector EffectiveCells = GetEffectiveCellSize();
	const FVector ExpectedSize = ShipBuilderGrid::CellSizeToWorldSize(EffectiveCells);
	if (Size.Equals(ExpectedSize, 1.0f) || Size.X <= 0.0f || Size.Y <= 0.0f || Size.Z <= 0.0f)
	{
		return;
	}

	const FIntVector FromSize = ShipBuilderGrid::WorldSizeToCellSize(Size);
	const FVector RoundedSize = ShipBuilderGrid::CellSizeToWorldSize(FromSize);
	if (!Size.Equals(RoundedSize, 1.0f))
	{
		return;
	}

	const bool bCellSizeDefault = EffectiveCells == FIntVector(1, 1, 1);
	const FVector DefaultOneCellSize = ShipBuilderGrid::CellSizeToWorldSize(FIntVector(1, 1, 1));
	const bool bSizeNonDefault = !Size.Equals(DefaultOneCellSize, 1.0f);
	if (bCellSizeDefault && bSizeNonDefault)
	{
		CellSize = FromSize;
	}
}

void UShipModuleDefinition::RegenerateDefaultContactPointsFromCellSize()
{
	if (HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
	{
		return;
	}

	if (const UShipModuleVisualOverride* Vo = GetVisualOverride())
	{
		if (Vo->bOverrideContactPoints && Vo->ContactPointsOverride.Num() > 0)
		{
			return;
		}
	}

	Modify();
	TArray<FShipModuleContactPoint> Defaults;
	AppendDefaultPanelContactPointsForCellSize(GetEffectiveCellSize(), Defaults);
	ContactPoints = MoveTemp(Defaults);
	MarkPackageDirty();
}

#endif // WITH_EDITOR

const UShipModuleVisualOverride* UShipModuleDefinition::GetVisualOverride() const
{
	return VisualOverride.IsNull() ? nullptr : VisualOverride.LoadSynchronous();
}

const TArray<FShipModuleContactPoint>& UShipModuleDefinition::GetResolvedContactPoints() const
{
	if (const UShipModuleVisualOverride* Override = GetVisualOverride())
	{
		if (Override->bOverrideContactPoints && Override->ContactPointsOverride.Num() > 0)
		{
			return Override->ContactPointsOverride;
		}
	}
	return ContactPoints;
}

void UShipModuleDefinition::GatherEffectiveContactPoints(TArray<FShipModuleContactPoint>& OutPoints) const
{
	OutPoints = GetResolvedContactPoints();
}

void UShipModuleDefinition::GatherContactPointsForPlacement(TArray<FShipModuleContactPoint>& OutPoints) const
{
	GatherEffectiveContactPoints(OutPoints);

	TArray<FShipModuleContactPoint> Defaults;
	AppendDefaultPanelContactPointsForCellSize(GetEffectiveCellSize(), Defaults);
	if (OutPoints.Num() == 0)
	{
		OutPoints = MoveTemp(Defaults);
		return;
	}

	TSet<FName> AuthoredNames;
	AuthoredNames.Reserve(OutPoints.Num());
	for (const FShipModuleContactPoint& Point : OutPoints)
	{
		AuthoredNames.Add(Point.SocketName);
	}
	for (const FShipModuleContactPoint& DefaultPoint : Defaults)
	{
		if (!AuthoredNames.Contains(DefaultPoint.SocketName))
		{
			OutPoints.Add(DefaultPoint);
		}
	}
}

void UShipModuleDefinition::EnsureContactPointsPopulatedIfNoAuthoringOverride()
{
	if (const UShipModuleVisualOverride* Vo = GetVisualOverride())
	{
		if (Vo->bOverrideContactPoints && Vo->ContactPointsOverride.Num() > 0)
		{
			return;
		}
	}
	if (ContactPoints.Num() > 0)
	{
		return;
	}
	if (Size.X <= 0.0f || Size.Y <= 0.0f || Size.Z <= 0.0f)
	{
		return;
	}
#if !WITH_EDITOR
	return;
#else
	if (HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
	{
		return;
	}
	Modify();
	TArray<FShipModuleContactPoint> Defaults;
	AppendDefaultPanelContactPointsForCellSize(GetEffectiveCellSize(), Defaults);
	ContactPoints = MoveTemp(Defaults);
	MarkPackageDirty();
#endif
}

void UShipModuleDefinition::AppendDefaultPanelContactPointsForCellSize(
	const FIntVector& InCellSize,
	TArray<FShipModuleContactPoint>& OutPoints)
{
	OutPoints.Reset();
	const FIntVector Cells = ShipBuilderGrid::GetEffectiveCellSize(InCellSize);
	const FVector WorldSize = ShipBuilderGrid::CellSizeToWorldSize(Cells);
	const FVector Half = WorldSize * 0.5f;

	auto PanelGridCenter = [&](const int32 IX, const int32 IY, const int32 IZ) -> FVector
	{
		return FVector(
			-Half.X + (static_cast<float>(IX) + 0.5f) * ShipBuilderGrid::PanelUnitXY,
			-Half.Y + (static_cast<float>(IY) + 0.5f) * ShipBuilderGrid::PanelUnitXY,
			-Half.Z + (static_cast<float>(IZ) + 0.5f) * ShipBuilderGrid::PanelUnitZ);
	};

	/** Центр сокета на внешней грани панели (±Half по оси грани). */
	auto FaceSocketLocation = [&](const TCHAR* FacePrefix, const int32 IX, const int32 IY, const int32 IZ) -> FVector
	{
		const FVector GridCenter = PanelGridCenter(IX, IY, IZ);
		if (FCString::Strcmp(FacePrefix, TEXT("Back")) == 0)
		{
			return FVector(-Half.X, GridCenter.Y, GridCenter.Z);
		}
		if (FCString::Strcmp(FacePrefix, TEXT("Front")) == 0)
		{
			return FVector(Half.X, GridCenter.Y, GridCenter.Z);
		}
		if (FCString::Strcmp(FacePrefix, TEXT("Left")) == 0)
		{
			return FVector(GridCenter.X, -Half.Y, GridCenter.Z);
		}
		if (FCString::Strcmp(FacePrefix, TEXT("Right")) == 0)
		{
			return FVector(GridCenter.X, Half.Y, GridCenter.Z);
		}
		if (FCString::Strcmp(FacePrefix, TEXT("Bottom")) == 0)
		{
			return FVector(GridCenter.X, GridCenter.Y, -Half.Z);
		}
		if (FCString::Strcmp(FacePrefix, TEXT("Top")) == 0)
		{
			return FVector(GridCenter.X, GridCenter.Y, Half.Z);
		}
		return GridCenter;
	};

	auto AddPanel = [&](const TCHAR* FacePrefix, const int32 IX, const int32 IY, const int32 IZ, const EShipModuleSocketType Type)
	{
		FShipModuleContactPoint CP;
		CP.SocketName = *FString::Printf(TEXT("%s_X%d_Y%d_Z%d"), FacePrefix, IX, IY, IZ);
		CP.RelativeLocation = FaceSocketLocation(FacePrefix, IX, IY, IZ);
		CP.SocketType = Type;
		OutPoints.Add(CP);
	};

	for (int32 IY = 0; IY < Cells.Y; ++IY)
	{
		for (int32 IZ = 0; IZ < Cells.Z; ++IZ)
		{
			AddPanel(TEXT("Back"), 0, IY, IZ, EShipModuleSocketType::Horizontal);
			AddPanel(TEXT("Front"), Cells.X - 1, IY, IZ, EShipModuleSocketType::Horizontal);
		}
	}
	for (int32 IX = 0; IX < Cells.X; ++IX)
	{
		for (int32 IZ = 0; IZ < Cells.Z; ++IZ)
		{
			AddPanel(TEXT("Left"), IX, 0, IZ, EShipModuleSocketType::Horizontal);
			AddPanel(TEXT("Right"), IX, Cells.Y - 1, IZ, EShipModuleSocketType::Horizontal);
		}
	}
	for (int32 IX = 0; IX < Cells.X; ++IX)
	{
		for (int32 IY = 0; IY < Cells.Y; ++IY)
		{
			AddPanel(TEXT("Bottom"), IX, IY, 0, EShipModuleSocketType::Vertical);
			AddPanel(TEXT("Top"), IX, IY, Cells.Z - 1, EShipModuleSocketType::Vertical);
		}
	}
}

void UShipModuleDefinition::AppendDefaultContactPointsForSize(
	const FVector& ModuleSize,
	TArray<FShipModuleContactPoint>& OutPoints)
{
	OutPoints.Reset();
	const FVector Half = ModuleSize * 0.5f;
	auto AddPoint = [&OutPoints](const TCHAR* Name, const FVector& Loc, const EShipModuleSocketType Type)
	{
		FShipModuleContactPoint CP;
		CP.SocketName = Name;
		CP.RelativeLocation = Loc;
		CP.SocketType = Type;
		OutPoints.Add(CP);
	};
	AddPoint(TEXT("Front"), FVector(Half.X, 0.0f, 0.0f), EShipModuleSocketType::Horizontal);
	AddPoint(TEXT("Back"), FVector(-Half.X, 0.0f, 0.0f), EShipModuleSocketType::Horizontal);
	AddPoint(TEXT("Left"), FVector(0.0f, -Half.Y, 0.0f), EShipModuleSocketType::Horizontal);
	AddPoint(TEXT("Right"), FVector(0.0f, Half.Y, 0.0f), EShipModuleSocketType::Horizontal);
	AddPoint(TEXT("Top"), FVector(0.0f, 0.0f, Half.Z), EShipModuleSocketType::Vertical);
	AddPoint(TEXT("Bottom"), FVector(0.0f, 0.0f, -Half.Z), EShipModuleSocketType::Vertical);
}

// ----------------------------------------------------------------------------
// Валидация (доступна и в рантайме, и в редакторе)
// ----------------------------------------------------------------------------

bool UShipModuleDefinition::Validate(TArray<FText>& OutErrors) const
{
	const int32 InitialCount = OutErrors.Num();

	if (ModuleId.IsNone())
	{
		OutErrors.Add(FText::FromString(TEXT("ModuleId обязателен (не может быть None).")));
	}

	if (DisplayName.IsEmpty())
	{
		OutErrors.Add(FText::FromString(TEXT("DisplayName обязателен (не может быть пустым).")));
	}

	if (Mass <= 0.0f)
	{
		OutErrors.Add(FText::FromString(
			FString::Printf(TEXT("Mass должна быть > 0 (текущее значение: %.2f)."), Mass)));
	}

	if (Size.X <= 0.0 || Size.Y <= 0.0 || Size.Z <= 0.0)
	{
		OutErrors.Add(FText::FromString(
			FString::Printf(TEXT("Все компоненты Size должны быть > 0 (текущее: %.1f, %.1f, %.1f)."),
				Size.X, Size.Y, Size.Z)));
	}

	TArray<FShipModuleContactPoint> EffectiveContactPoints;
	GatherEffectiveContactPoints(EffectiveContactPoints);
	if (EffectiveContactPoints.Num() == 0)
	{
		OutErrors.Add(FText::FromString(TEXT("Нужна хотя бы одна контактная точка: задайте ContactPoints на определении или ContactPointsOverride в VisualOverride (bOverrideContactPoints).")));
	}
	else
	{
		TSet<FName> SeenNames;
		for (int32 i = 0; i < EffectiveContactPoints.Num(); ++i)
		{
			const FShipModuleContactPoint& CP = EffectiveContactPoints[i];

			if (CP.SocketName.IsNone())
			{
				OutErrors.Add(FText::FromString(
					FString::Printf(TEXT("ContactPoints[%d]: SocketName обязателен (не может быть None)."), i)));
			}
			else
			{
				bool bAlreadyInSet = false;
				SeenNames.Add(CP.SocketName, &bAlreadyInSet);
				if (bAlreadyInSet)
				{
					OutErrors.Add(FText::FromString(
						FString::Printf(TEXT("ContactPoints[%d]: дублирующийся SocketName '%s'."),
							i, *CP.SocketName.ToString())));
				}
			}
		}
	}

	return OutErrors.Num() == InitialCount;
}

// ----------------------------------------------------------------------------
// Editor-only: валидация данных и обратная связь
// ----------------------------------------------------------------------------

#if WITH_EDITOR

EDataValidationResult UShipModuleDefinition::IsDataValid(FDataValidationContext& Context) const
{
	TArray<FText> Errors;
	if (Validate(Errors))
	{
		return EDataValidationResult::Valid;
	}

	for (const FText& Err : Errors)
	{
		Context.AddError(Err);
	}
	return EDataValidationResult::Invalid;
}

void UShipModuleDefinition::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName CellSizePropertyName = GET_MEMBER_NAME_CHECKED(UShipModuleDefinition, CellSize);
	const FName PropertyName = PropertyChangedEvent.Property
		? PropertyChangedEvent.Property->GetFName()
		: NAME_None;
	const FName MemberPropertyName = PropertyChangedEvent.MemberProperty
		? PropertyChangedEvent.MemberProperty->GetFName()
		: NAME_None;
	if (PropertyName == CellSizePropertyName || MemberPropertyName == CellSizePropertyName)
	{
		SyncCellSizeAndSizeFromLegacy();
		RegenerateDefaultContactPointsFromCellSize();
	}

	TArray<FText> Errors;
	if (!Validate(Errors))
	{
		FMessageLog MessageLog("AssetCheck");
		for (const FText& Err : Errors)
		{
			MessageLog.Warning(Err);
		}
		MessageLog.Open(EMessageSeverity::Warning);
	}
}

void UShipModuleDefinition::PostLoad()
{
	Super::PostLoad();
	const FVector SizeBefore = Size;
	MigrateLegacySizeToCellSizeIfNeeded();
	SyncCellSizeAndSizeFromLegacy();

	TArray<FShipModuleContactPoint> ExpectedPanels;
	AppendDefaultPanelContactPointsForCellSize(GetEffectiveCellSize(), ExpectedPanels);
	const bool bHasContactOverride = [this]() -> bool
	{
		if (const UShipModuleVisualOverride* Vo = GetVisualOverride())
		{
			return Vo->bOverrideContactPoints && Vo->ContactPointsOverride.Num() > 0;
		}
		return false;
	}();
	if (!bHasContactOverride
		&& (!SizeBefore.Equals(Size, 1.0f) || ContactPoints.Num() != ExpectedPanels.Num()))
	{
		RegenerateDefaultContactPointsFromCellSize();
	}

	EnsureContactPointsPopulatedIfNoAuthoringOverride();
}

/** При дублировании сбрасываем ModuleId, чтобы пользователь задал уникальный. */
void UShipModuleDefinition::PostDuplicate(bool bDuplicateForPIE)
{
	Super::PostDuplicate(bDuplicateForPIE);

	if (!bDuplicateForPIE)
	{
		ModuleId = NAME_None;
		UE_LOG(LogShipModule, Log, TEXT("Модуль продублирован — ModuleId сброшен. Задайте новый уникальный идентификатор."));
	}
}

#endif // WITH_EDITOR
