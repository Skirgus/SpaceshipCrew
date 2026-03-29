// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ShipInteractableBase.h"
#include "ShipReactorStation.generated.h"

/**
 * Станция реактора с корректными умолчаниями для C++ и BP.
 * Переназначьте родителя у реактора в уровне с ShipInteractableBase на этот класс (или задайте ActionId = AdjustReactor вручную).
 */
UCLASS()
class SPACESHIPCREW_API AShipReactorStation : public AShipInteractableBase
{
	GENERATED_BODY()

public:
	AShipReactorStation();
};
