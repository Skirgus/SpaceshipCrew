#pragma once

#include "CoreMinimal.h"
#include "SpaceshipCrewMenuWidgetBase.h"
#include "ShipBuilderWidgetBases.generated.h"

/**
 * Базовый UMG-класс корневого экрана конструктора; визуальная сборка — в WBP-наследнике (T02c-1).
 */
UCLASS(Abstract, Blueprintable, BlueprintType)
class SPACESHIPCREW_API UShipBuilderRootWidgetBase : public USpaceshipCrewMenuWidgetBase
{
	GENERATED_BODY()
};

/**
 * Базовый виджет чеклиста (ошибки / предупреждения).
 */
UCLASS(Abstract, Blueprintable, BlueprintType)
class SPACESHIPCREW_API UShipBuilderChecklistWidgetBase : public USpaceshipCrewMenuWidgetBase
{
	GENERATED_BODY()
};

/**
 * Базовый виджет каталога модулей.
 */
UCLASS(Abstract, Blueprintable, BlueprintType)
class SPACESHIPCREW_API UShipBuilderModuleCatalogWidgetBase : public USpaceshipCrewMenuWidgetBase
{
	GENERATED_BODY()
};
