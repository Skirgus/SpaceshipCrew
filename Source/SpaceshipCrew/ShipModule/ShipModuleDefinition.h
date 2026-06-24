#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ShipModuleTypes.h"
#include "ShipModuleDefinition.generated.h"

/**
 * Определение модуля корабля — единица данных конструктора.
 *
 * Каждый экземпляр этого ассета описывает один тип модуля: его назначение,
 * физические характеристики, контактные точки для стыковки и правила совместимости.
 * Ассеты обнаруживаются автоматически через AssetManager (PrimaryAssetType "ShipModule")
 * и доступны в рантайме через UShipModuleCatalog.
 *
 * Создание: Content Browser → ПКМ → Miscellaneous → Ship Module Definition.
 * Хранение: Content/Data/ShipModules/ (в редакторе /Game/Data/ShipModules), совпадает с PrimaryAssetTypesToScan в DefaultEngine.ini.
 * Дублирование: Ctrl+D / ПКМ → Duplicate (ModuleId сбрасывается автоматически).
 */
UCLASS(BlueprintType)
class SPACESHIPCREW_API UShipModuleDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Уникальный идентификатор модуля (обязательный, не может быть NAME_None). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FName ModuleId;

	/** Функциональный тип модуля. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	EShipModuleType ModuleType = EShipModuleType::Corridor;

	/** Локализованное отображаемое название (обязательно). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FText DisplayName;

	/** Описание модуля для UI (необязательно). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FText Description;

	/** Масса модуля в кг (обязательно > 0). Влияет на расход топлива и маневренность. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Physics", meta = (ClampMin = "0.01"))
	float Mass = 100.0f;

	/** Стоимость в кредитах для UI конструктора. Если 0 — для суммы используется округлённая масса. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Economy", meta = (ClampMin = "0"))
	int32 CreditCost = 0;

	/** Габариты модуля в см (производное от CellSize, синхронизируется автоматически). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Physics")
	FVector Size = FVector(400.0, 400.0, 300.0);

	/**
	 * Размер модуля в панелях сетки (1 панель = 400×400×300 см).
	 * Модуль 2×1×2 = две панели по X, одна по Y, два этажа по Z.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Physics", meta = (ClampMin = "1"))
	FIntVector CellSize = FIntVector(1, 1, 1);

	/** Есть ли внутренний объём, по которому может перемещаться экипаж. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interior")
	bool bHasInterior = false;

	/**
	 * Принудительная сторона проёма в оболочке модуля (MVP-визуал).
	 * В первую очередь используется для Airlock: выбранная сторона отображается как проём,
	 * даже если к ней не пристыкован соседний модуль.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interior")
	EShipModuleOpeningSide ForcedOpeningSide = EShipModuleOpeningSide::None;

	/**
	 * Контактные точки стыковки (данные). Маркеры «сокетов» в билдере и сторителе рисуются только по этому списку,
	 * пока в VisualOverride не включён отдельный список ContactPointsOverride (bOverrideContactPoints).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Docking")
	TArray<FShipModuleContactPoint> ContactPoints;

	/**
	 * Опциональный ручной визуальный override модуля.
	 * Если назначен и содержит VisualParts, билдер отображает модуль по этим данным.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TSoftObjectPtr<class UShipModuleVisualOverride> VisualOverride;

	/** Типы модулей, которые могут стыковаться с данным модулем. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Docking")
	TArray<EShipModuleType> CompatibleModuleTypes;

	/** Эффективная стоимость для суммы в конструкторе (CreditCost или оценка по массе). */
	int32 GetEffectiveCreditCost() const;

	/** Эффективный размер в панелях (минимум 1 по каждой оси). */
	FIntVector GetEffectiveCellSize() const;

	/** Пересчитывает Size из CellSize (CellSize — источник истины в редакторе). */
	void SyncCellSizeAndSizeFromLegacy();

#if WITH_EDITOR
	/** PostLoad: если CellSize ещё 1×1×1, а Size задан legacy-габаритом — вывести CellSize из Size. */
	void MigrateLegacySizeToCellSizeIfNeeded();

	/** Заменяет ContactPoints на panel-сокеты текущего CellSize (если нет override в VisualOverride). */
	void RegenerateDefaultContactPointsFromCellSize();
#endif

	/** Резолв: ContactPointsOverride при bOverrideContactPoints и непустом списке, иначе ContactPoints на определении. */
	const TArray<FShipModuleContactPoint>& GetResolvedContactPoints() const;

	/**
	 * Шесть стыковочных точек на гранях bbox (см). Устаревший fallback для 1×1×1.
	 */
	static void AppendDefaultContactPointsForSize(const FVector& ModuleSize, TArray<FShipModuleContactPoint>& OutPoints);

	/** Сокеты по центру каждой внешней панели грани (CellSize панелей). */
	static void AppendDefaultPanelContactPointsForCellSize(
		const FIntVector& InCellSize,
		TArray<FShipModuleContactPoint>& OutPoints);

	/**
	 * Эффективные контактные точки для домена/превью: копия GetResolvedContactPoints (без «виртуальных» сокетов).
	 */
	void GatherEffectiveContactPoints(TArray<FShipModuleContactPoint>& OutPoints) const;

	/**
	 * Сокеты для стыковки и превью: authored-точки + недостающие грани bbox по Size.
	 * Частичный override в VisualOverride не блокирует стыковку по незаданным граням.
	 */
	void GatherContactPointsForPlacement(TArray<FShipModuleContactPoint>& OutPoints) const;

	/**
	 * Если нет authoring-сокетов в VisualOverride и ContactPoints пуст — добавить шесть граней по Size (только редактор).
	 * Вызывается из PostLoad и может вызываться из инструментов/тестов.
	 */
	void EnsureContactPointsPopulatedIfNoAuthoringOverride();

	/** Загруженный visual override (если задан). */
	const class UShipModuleVisualOverride* GetVisualOverride() const;

	// --- UPrimaryDataAsset ---

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	// --- Валидация ---

	/**
	 * Проверяет корректность всех обязательных полей и контактных точек.
	 * Вызывается при сохранении ассета и при запуске Data Validation.
	 * Возвращает true если модуль валиден.
	 */
	bool Validate(TArray<FText>& OutErrors) const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
	virtual void PostLoad() override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostDuplicate(bool bDuplicateForPIE) override;
#endif
};
