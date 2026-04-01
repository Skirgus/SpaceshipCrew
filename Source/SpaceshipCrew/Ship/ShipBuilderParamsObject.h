// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ShipBuilderParamsObject.generated.h"

class UShipConfigAsset;
class UShipModuleDefinition;

UCLASS(BlueprintType)
class SPACESHIPCREW_API UShipBuilderParamsObject : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship Builder")
	TObjectPtr<UShipConfigAsset> StarterConfig = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship Builder")
	TObjectPtr<UShipModuleDefinition> ReactorDef = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship Builder")
	TObjectPtr<UShipModuleDefinition> EngineDef = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship Builder")
	TObjectPtr<UShipModuleDefinition> BridgeDef = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship Builder")
	TObjectPtr<UShipModuleDefinition> OxygenTankDef = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship Builder")
	TObjectPtr<UShipModuleDefinition> FuelTankDef = nullptr;
};

