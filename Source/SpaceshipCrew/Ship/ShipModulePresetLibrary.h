// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ShipConfigAsset.h"
#include "ShipModuleDefinition.h"
#include "ShipModulePresetLibrary.generated.h"

UENUM(BlueprintType)
enum class EShipModulePresetKind : uint8
{
	Reactor,
	Engine,
	Bridge,
	Airlock,
	OxygenTank,
	FuelTank
};

UCLASS()
class SPACESHIPCREW_API UShipModulePresetLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Ship|Module|Presets")
	static bool ApplyPresetToDefinition(UShipModuleDefinition* Definition, EShipModulePresetKind PresetKind);

	UFUNCTION(BlueprintCallable, Category = "Ship|Config|Presets")
	static bool BuildStarterShipConfig(
		UShipConfigAsset* Config,
		UShipModuleDefinition* ReactorDefinition,
		UShipModuleDefinition* EngineDefinition,
		UShipModuleDefinition* BridgeDefinition,
		UShipModuleDefinition* AirlockDefinition,
		UShipModuleDefinition* OxygenTankDefinition,
		UShipModuleDefinition* FuelTankDefinition);
};

