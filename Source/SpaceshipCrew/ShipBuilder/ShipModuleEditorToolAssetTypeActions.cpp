#if WITH_EDITOR

#include "ShipBuilder/ShipModuleEditorToolAssetTypeActions.h"

#include "ShipBuilder/ShipModuleEditorTool.h"
#include "ShipBuilder/ShipModuleEditorToolAssetEditor.h"

FText FShipModuleEditorToolAssetTypeActions::GetName() const
{
	return FText::FromString(TEXT("Ship Module Editor Tool"));
}

FColor FShipModuleEditorToolAssetTypeActions::GetTypeColor() const
{
	return FColor(70, 160, 255);
}

UClass* FShipModuleEditorToolAssetTypeActions::GetSupportedClass() const
{
	return UShipModuleEditorTool::StaticClass();
}

uint32 FShipModuleEditorToolAssetTypeActions::GetCategories()
{
	return EAssetTypeCategories::Misc;
}

void FShipModuleEditorToolAssetTypeActions::OpenAssetEditor(
	const TArray<UObject*>& InObjects,
	TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	const EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;
	for (UObject* Obj : InObjects)
	{
		if (UShipModuleEditorTool* ToolAsset = Cast<UShipModuleEditorTool>(Obj))
		{
			TSharedRef<FShipModuleEditorToolAssetEditor> Editor = MakeShared<FShipModuleEditorToolAssetEditor>();
			Editor->InitEditor(Mode, EditWithinLevelEditor, ToolAsset);
		}
	}
}

#endif // WITH_EDITOR

