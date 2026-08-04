#if WITH_EDITOR

#include "ShipModuleDefinitionFactory.h"
#include "ShipModuleDefinition.h"
#include "ShipBuilder/ShipBuilderGridConstants.h"
#include "AssetToolsModule.h"

UShipModuleDefinitionFactory::UShipModuleDefinitionFactory()
{
	SupportedClass = UShipModuleDefinition::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* UShipModuleDefinitionFactory::FactoryCreateNew(
	UClass* InClass,
	UObject* InParent,
	FName InName,
	EObjectFlags Flags,
	UObject* Context,
	FFeedbackContext* Warn)
{
	UShipModuleDefinition* NewModule = NewObject<UShipModuleDefinition>(InParent, InClass, InName, Flags);

	NewModule->Mass = 100.0f;
	NewModule->CellSize = FIntVector(1, 1, 1);
	NewModule->Size = ShipBuilderGrid::CellSizeToWorldSize(NewModule->CellSize);

	UShipModuleDefinition::AppendDefaultPanelContactPointsForCellSize(NewModule->CellSize, NewModule->ContactPoints);

	return NewModule;
}

FText UShipModuleDefinitionFactory::GetDisplayName() const
{
	return FText::FromString(TEXT("Ship Module Definition"));
}

uint32 UShipModuleDefinitionFactory::GetMenuCategories() const
{
	return EAssetTypeCategories::Misc;
}

#endif // WITH_EDITOR
