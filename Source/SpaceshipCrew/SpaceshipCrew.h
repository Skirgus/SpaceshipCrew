#pragma once

/**
 * Модуль SpaceshipCrew: игровой код демо MVP.
 * Макрос SPACESHIPCREW_API используется для экспорта символов в UCLASS и публичных типах модуля.
 */
#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

#ifndef SPACESHIPCREW_API
#define SPACESHIPCREW_API
#endif

class FAssetTypeActions_Base;

/**
 * Модуль SpaceshipCrew: runtime + editor расширения.
 */
class SPACESHIPCREW_API FSpaceshipCrewModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
#if WITH_EDITOR
	TSharedPtr<FAssetTypeActions_Base> ShipModuleEditorToolAssetTypeActions;
#endif
};
