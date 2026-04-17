#pragma once

#include "CoreMinimal.h"

/**
 * Черновик конфигурации конструктора без интерактивной стыковки (T02c-1).
 * Порядок модулей задаёт автоматическую цепочку для вызова T02b (см. ShipBuilderDomainGlue).
 */
struct FShipBuilderDraftConfig
{
	TArray<FName> ModuleIds;
};
