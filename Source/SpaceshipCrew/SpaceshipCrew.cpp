#include "SpaceshipCrew.h"
#include "Modules/ModuleManager.h"

#if WITH_EDITOR
#include "AssetToolsModule.h"
#include "ShipBuilder/ShipModuleEditorToolAssetTypeActions.h"
#endif

void FSpaceshipCrewModule::StartupModule()
{
#if WITH_EDITOR
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	ShipModuleEditorToolAssetTypeActions = MakeShared<FShipModuleEditorToolAssetTypeActions>();
	AssetTools.RegisterAssetTypeActions(ShipModuleEditorToolAssetTypeActions.ToSharedRef());
#endif
}

void FSpaceshipCrewModule::ShutdownModule()
{
#if WITH_EDITOR
	if (ShipModuleEditorToolAssetTypeActions.IsValid() && FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		AssetTools.UnregisterAssetTypeActions(ShipModuleEditorToolAssetTypeActions.ToSharedRef());
	}
	ShipModuleEditorToolAssetTypeActions.Reset();
#endif
}

/** Точка входа runtime-модуля игры. */
IMPLEMENT_PRIMARY_GAME_MODULE(FSpaceshipCrewModule, SpaceshipCrew, "SpaceshipCrew");
