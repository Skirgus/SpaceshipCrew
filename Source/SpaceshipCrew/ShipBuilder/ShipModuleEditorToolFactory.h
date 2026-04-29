#pragma once

#if WITH_EDITOR

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "ShipModuleEditorToolFactory.generated.h"

/**
 * Фабрика для создания ассетов UShipModuleEditorTool в Content Browser.
 */
UCLASS()
class UShipModuleEditorToolFactory : public UFactory
{
	GENERATED_BODY()

public:
	UShipModuleEditorToolFactory();

	virtual UObject* FactoryCreateNew(
		UClass* InClass,
		UObject* InParent,
		FName InName,
		EObjectFlags Flags,
		UObject* Context,
		FFeedbackContext* Warn) override;

	virtual bool ShouldShowInNewMenu() const override { return true; }
	virtual FText GetDisplayName() const override;
	virtual uint32 GetMenuCategories() const override;
};

#endif // WITH_EDITOR

