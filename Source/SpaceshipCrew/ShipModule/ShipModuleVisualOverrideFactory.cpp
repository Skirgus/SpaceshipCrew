#if WITH_EDITOR

#include "ShipModuleVisualOverrideFactory.h"
#include "ShipModuleVisualOverride.h"
#include "AssetToolsModule.h"

UShipModuleVisualOverrideFactory::UShipModuleVisualOverrideFactory()
{
	SupportedClass = UShipModuleVisualOverride::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* UShipModuleVisualOverrideFactory::FactoryCreateNew(
	UClass* InClass,
	UObject* InParent,
	FName InName,
	EObjectFlags Flags,
	UObject* Context,
	FFeedbackContext* Warn)
{
	return NewObject<UShipModuleVisualOverride>(InParent, InClass, InName, Flags);
}

FText UShipModuleVisualOverrideFactory::GetDisplayName() const
{
	return FText::FromString(TEXT("Ship Module Visual Override"));
}

uint32 UShipModuleVisualOverrideFactory::GetMenuCategories() const
{
	return EAssetTypeCategories::Misc;
}

#endif // WITH_EDITOR

