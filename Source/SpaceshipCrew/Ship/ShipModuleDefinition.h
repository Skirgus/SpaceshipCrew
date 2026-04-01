// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ShipSystemsComponent.h"
#include "ShipModuleDefinition.generated.h"

class UStaticMesh;

UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EShipModuleCapability : uint8
{
	None = 0 UMETA(Hidden),
	ReactorCore = 1 << 0,
	EngineCore = 1 << 1,
	BridgeCore = 1 << 2,
	AirlockCore = 1 << 3,
	OxygenStorage = 1 << 4,
	FuelStorage = 1 << 5
};
ENUM_CLASS_FLAGS(EShipModuleCapability)

UENUM(BlueprintType)
enum class EShipModuleFace : uint8
{
	PosX,
	NegX,
	PosY,
	NegY,
	PosZ,
	NegZ
};

USTRUCT(BlueprintType)
struct SPACESHIPCREW_API FShipModuleConnectionPoint
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Module|Connections")
	EShipModuleFace Face = EShipModuleFace::PosX;

	/** Нормализованная позиция вдоль ребра стороны (0..1). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Module|Connections", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Slot = 0.5f;
};

UCLASS(BlueprintType)
class SPACESHIPCREW_API UShipModuleDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Module")
	FName ModuleDefinitionId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Module")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Module")
	EShipModuleType ModuleType = EShipModuleType::Custom;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Module", meta = (Bitmask, BitmaskEnum = "/Script/SpaceshipCrew.EShipModuleCapability"))
	int32 CapabilitiesMask = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Module")
	FIntVector GridSize = FIntVector(1, 1, 1);

	/** Есть ли у модуля внутренний объем-комната. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Module")
	bool bHasInteriorVolume = true;

	/** Для внешних модулей без объема: разрешить точки соединения на всех сторонах. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Module")
	bool bAllowConnectionsOnAllSides = false;

	/** Явные точки соединения. Если пусто, строятся автоматически по GridSize и флагам. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Module|Connections")
	TArray<FShipModuleConnectionPoint> ConnectionPoints;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Module|Visual")
	TSoftObjectPtr<UStaticMesh> ModuleMesh;

	UFUNCTION(BlueprintPure, Category = "Ship|Module")
	bool HasCapability(EShipModuleCapability Capability) const;

	UFUNCTION(BlueprintPure, Category = "Ship|Module|Connections")
	TArray<FShipModuleConnectionPoint> GetEffectiveConnectionPoints() const;
};

UCLASS(BlueprintType)
class SPACESHIPCREW_API UShipModuleCatalog : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Module")
	TArray<TSoftObjectPtr<UShipModuleDefinition>> Modules;
};

