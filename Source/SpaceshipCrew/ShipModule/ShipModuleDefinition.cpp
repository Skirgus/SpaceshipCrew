#include "ShipModuleDefinition.h"
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
	AppendDefaultContactPointsForSize(Size, Defaults);
	ContactPoints = MoveTemp(Defaults);
	MarkPackageDirty();
#endif
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
