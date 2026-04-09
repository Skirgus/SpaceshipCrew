#if WITH_EDITOR

#include "ShipModuleDefinitionFactory.h"
#include "ShipModuleDefinition.h"
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
	NewModule->Size = FVector(400.0, 400.0, 300.0);

	FShipModuleContactPoint DefaultCP;
	DefaultCP.SocketName = FName(TEXT("Front"));
	DefaultCP.RelativeLocation = FVector(200.0, 0.0, 0.0);
	DefaultCP.SocketType = EShipModuleSocketType::Horizontal;
	NewModule->ContactPoints.Add(DefaultCP);

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
