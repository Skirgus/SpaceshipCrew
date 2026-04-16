#pragma once

#include "CoreMinimal.h"

#include "ShipBuilderDraftTypes.h"

class IShipBuildModuleResolver;
class FShipBuildDomainModel;

/**
 * Строит линейную цепочку модулей по порядку в черновике (первый — корень, далее стык к предыдущему через первый контактный узел каждого определения).
 * Нужен для проверки T02b без UI стыковки.
 *
 * @return false если не удалось собрать цепочку; иначе OutModel заполнен.
 */
bool SpaceshipCrew_BuildDomainFromDraftChain(
	const FShipBuilderDraftConfig& Draft,
	const IShipBuildModuleResolver& Resolver,
	FShipBuildDomainModel& OutModel,
	FString& OutError);
