#pragma once

#include "CoreMinimal.h"

#include "ShipBuilder/ShipBuilderDraftTypes.h"

class IShipBuildModuleResolver;
class FShipBuildDomainModel;

/**
 * Строит доменную конфигурацию из draft.
 * - Если в draft есть явные PlacedModules/Connections, использует их.
 * - Иначе работает fallback-логика legacy-цепочки по ModuleIds.
 */
bool SpaceshipCrew_BuildDomainFromDraftChain(
	const FShipBuilderDraftConfig& Draft,
	const IShipBuildModuleResolver& Resolver,
	FShipBuildDomainModel& OutModel,
	FString& OutError);
