#pragma once

#include "CoreMinimal.h"
#include "ShipBuilderDraftTypes.generated.h"

/**
 * Размещённый модуль в черновике конструктора.
 */
USTRUCT(BlueprintType)
struct SPACESHIPCREW_API FShipBuilderPlacedModule
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShipBuilder")
	FName InstanceId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShipBuilder")
	FName ModuleId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShipBuilder")
	FIntVector GridPos = FIntVector::ZeroValue;

	/** 0..3, итоговый yaw = YawStep * 90°. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShipBuilder")
	int32 YawStep = 0;
};

/**
 * Стыковка двух модулей в черновике.
 */
USTRUCT(BlueprintType)
struct SPACESHIPCREW_API FShipBuilderDraftConnection
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShipBuilder")
	FName ModuleAInstanceId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShipBuilder")
	FName ModuleASocketName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShipBuilder")
	FName ModuleBInstanceId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShipBuilder")
	FName ModuleBSocketName = NAME_None;
};

/**
 * Черновик конфигурации конструктора (T02c).
 * Порядок ModuleIds — legacy; для T02b предпочтительны PlacedModules и Connections.
 */
USTRUCT(BlueprintType)
struct SPACESHIPCREW_API FShipBuilderDraftConfig
{
	GENERATED_BODY()

	/** Legacy-порядок для совместимости с UI и оценщиками. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShipBuilder")
	TArray<FName> ModuleIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShipBuilder")
	TArray<FShipBuilderPlacedModule> PlacedModules;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShipBuilder")
	TArray<FShipBuilderDraftConnection> Connections;

	/** Совместимость со старым вложенным именованием. */
	using FPlacedModule = FShipBuilderPlacedModule;
	using FConnection = FShipBuilderDraftConnection;
};
