#pragma once

#include "CoreMinimal.h"

/**
 * Черновик конфигурации конструктора без интерактивной стыковки (T02c-1).
 * Порядок модулей задаёт автоматическую цепочку для вызова T02b (см. ShipBuilderDomainGlue).
 */
struct FShipBuilderDraftConfig
{
	struct FPlacedModule
	{
		FName InstanceId = NAME_None;
		FName ModuleId = NAME_None;
		FIntVector GridPos = FIntVector::ZeroValue;
		int32 YawStep = 0; // 0..3, where step*90 is final yaw.
	};

	struct FConnection
	{
		FName ModuleAInstanceId = NAME_None;
		FName ModuleASocketName = NAME_None;
		FName ModuleBInstanceId = NAME_None;
		FName ModuleBSocketName = NAME_None;
	};

	// Legacy order kept for compatibility with older UI/evaluators.
	TArray<FName> ModuleIds;
	TArray<FPlacedModule> PlacedModules;
	TArray<FConnection> Connections;
};
