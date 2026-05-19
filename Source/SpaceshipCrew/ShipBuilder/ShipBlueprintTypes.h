#pragma once

#include "CoreMinimal.h"
#include "ShipBuilder/ShipBuilderDraftTypes.h"
#include "ShipBlueprintTypes.generated.h"

/** Источник чертежа корабля. */
UENUM(BlueprintType)
enum class EShipBlueprintSource : uint8
{
	/** Встроенный шаблон проекта (Data Asset или JSON в Content). */
	Project,
	/** Сохранение игрока в Saved/ShipBlueprints. */
	Player
};

/** Режим редактирования в сессии конструктора. */
UENUM(BlueprintType)
enum class EShipBlueprintEditSource : uint8
{
	New,
	ProjectTemplate,
	PlayerOwned
};

/** Задел: объект внутри модуля (v1 не используется). */
USTRUCT(BlueprintType)
struct SPACESHIPCREW_API FShipInteriorPlacement
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShipBlueprint")
	FName ModuleInstanceId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShipBlueprint")
	FName ObjectId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShipBlueprint")
	FTransform LocalTransform = FTransform::Identity;
};

/**
 * Версионируемый документ чертежа корабля — единый контракт для Content, JSON и T04.
 */
USTRUCT(BlueprintType)
struct SPACESHIPCREW_API FShipBlueprintDocument
{
	GENERATED_BODY()

	static constexpr int32 CurrentSchemaVersion = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShipBlueprint")
	int32 SchemaVersion = CurrentSchemaVersion;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShipBlueprint")
	FName ShipId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShipBlueprint")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShipBlueprint")
	FShipBuilderDraftConfig ModuleLayout;

	/** Зарезервировано под предметы внутри модулей. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShipBlueprint")
	TArray<FShipInteriorPlacement> InteriorPlacements;

	/** Кэш стоимости для будущего budget gate (T04). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShipBlueprint")
	int32 CachedCreditCost = 0;
};

/** Краткая запись в списке выбора корабля. */
USTRUCT(BlueprintType)
struct SPACESHIPCREW_API FShipBlueprintListEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShipBlueprint")
	EShipBlueprintSource Source = EShipBlueprintSource::Project;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShipBlueprint")
	FName ShipId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShipBlueprint")
	FText DisplayName;

	/** true: чертёж проходит ValidateDocumentForPlay (готов к новой игре). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShipBlueprint")
	bool bPlayReady = false;
};

/** Состояние текущей сессии конструктора (хранится в подсистеме GameInstance). */
USTRUCT(BlueprintType)
struct SPACESHIPCREW_API FShipBlueprintSession
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShipBlueprint")
	EShipBlueprintEditSource EditSource = EShipBlueprintEditSource::New;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShipBlueprint")
	FName SourceShipId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShipBlueprint")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShipBlueprint")
	bool bDirty = false;

	/** true для ProjectTemplate: обычное Save недоступно, только Save As. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShipBlueprint")
	bool bRequiresSaveAsOnWrite = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShipBlueprint")
	FShipBlueprintDocument PendingDocument;

	bool HasLoadedDocument() const
	{
		return EditSource != EShipBlueprintEditSource::New
			|| PendingDocument.ModuleLayout.PlacedModules.Num() > 0;
	}
};
