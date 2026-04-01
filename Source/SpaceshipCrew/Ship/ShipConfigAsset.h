// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ShipSystemsComponent.h"
#include "ShipConfigAsset.generated.h"

class UShipModuleDefinition;

USTRUCT(BlueprintType)
struct SPACESHIPCREW_API FShipModuleConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Config")
	FName ModuleId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Config")
	TSoftObjectPtr<UShipModuleDefinition> ModuleDefinition;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Config")
	EShipModuleType ModuleType = EShipModuleType::Custom;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Config")
	bool bMandatory = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Config")
	FIntVector GridPosition = FIntVector::ZeroValue;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Config", meta = (ClampMin = "0.0", ClampMax = "30.0"))
	float OxygenLevel = 21.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Config", meta = (ClampMin = "0", ClampMax = "12"))
	int32 FireHotspotCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Config", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BreachSeverity = 0.f;
};

USTRUCT(BlueprintType)
struct SPACESHIPCREW_API FShipConnectionConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Config")
	FName ModuleA = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Config")
	FName ModuleB = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Config")
	bool bOpenByDefault = true;
};

UCLASS(BlueprintType)
class SPACESHIPCREW_API UShipConfigAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Config")
	FName ConfigId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Config")
	TArray<FShipModuleConfig> Modules;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Config")
	TArray<FShipConnectionConfig> Connections;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Config", meta = (ClampMin = "0.0"))
	float OxygenReserve = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Config", meta = (ClampMin = "0.0"))
	float FuelReserve = 100.f;

	UFUNCTION(BlueprintCallable, Category = "Ship|Config")
	bool ValidateConfig(TArray<FText>& OutErrors) const;
};

