#pragma once

#if WITH_EDITOR

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "ShipModuleDefinitionFactory.generated.h"

/**
 * Фабрика для создания ассетов UShipModuleDefinition через Content Browser.
 * Добавляет пункт «Ship Module Definition» в меню Create Advanced Asset → Miscellaneous.
 * Создаёт ассет с разумными дефолтами: масса 100 кг, габариты 400x400x300 см,
 * одна контактная точка «Front» горизонтального типа.
 */
UCLASS()
class UShipModuleDefinitionFactory : public UFactory
{
	GENERATED_BODY()

public:
	UShipModuleDefinitionFactory();

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
