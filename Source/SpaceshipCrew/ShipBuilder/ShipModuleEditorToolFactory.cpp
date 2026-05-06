#if WITH_EDITOR

#include "ShipBuilder/ShipModuleEditorToolFactory.h"
#include "ShipBuilder/ShipModuleEditorTool.h"
#include "AssetToolsModule.h"
#include "Engine/StaticMesh.h"

UShipModuleEditorToolFactory::UShipModuleEditorToolFactory()
{
	SupportedClass = UShipModuleEditorTool::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* UShipModuleEditorToolFactory::FactoryCreateNew(
	UClass* InClass,
	UObject* InParent,
	FName InName,
	EObjectFlags Flags,
	UObject* Context,
	FFeedbackContext* Warn)
{
	UShipModuleEditorTool* Tool = NewObject<UShipModuleEditorTool>(InParent, InClass, InName, Flags);
	if (UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")))
	{
		Tool->DefaultPartMesh = CubeMesh;
	}
	return Tool;
}

FText UShipModuleEditorToolFactory::GetDisplayName() const
{
	return FText::FromString(TEXT("Ship Module Editor Tool"));
}

uint32 UShipModuleEditorToolFactory::GetMenuCategories() const
{
	return EAssetTypeCategories::Misc;
}

#endif // WITH_EDITOR

