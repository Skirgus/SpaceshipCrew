#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameFramework/Actor.h"
#include "ShipModuleTypes.h"
#include "ShipModuleVisualOverride.generated.h"

/**
 * Один визуальный элемент кастомного представления модуля.
 * Transform задаётся в локальных координатах модуля (центр модуля = 0,0,0).
 */
USTRUCT(BlueprintType)
struct FShipModuleVisualPart
{
	GENERATED_BODY()

	/** Меш элемента. Если не задан — запись игнорируется. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TObjectPtr<UStaticMesh> Mesh = nullptr;

	/** Опциональный actor class для элемента (например, кресло/консоль как Blueprint Actor). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TSoftClassPtr<AActor> ActorClass;

	/** Локальный transform элемента относительно центра модуля. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	FTransform RelativeTransform = FTransform::Identity;
};

/**
 * Ручной визуальный override для ModuleDefinition.
 *
 * Если у модуля назначен этот ассет и в нём есть VisualParts, билдер использует
 * их вместо процедурной "коробки". Также ассет может переопределять контактные точки.
 */
UCLASS(BlueprintType)
class SPACESHIPCREW_API UShipModuleVisualOverride : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Набор вручную отредактированных мешей для модуля. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TArray<FShipModuleVisualPart> VisualParts;

	/** Использовать контактные точки из этого ассета вместо ModuleDefinition.ContactPoints. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Docking")
	bool bOverrideContactPoints = false;

	/** Контактные точки override (используются только при bOverrideContactPoints=true). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Docking", meta = (EditCondition = "bOverrideContactPoints"))
	TArray<FShipModuleContactPoint> ContactPointsOverride;
};

