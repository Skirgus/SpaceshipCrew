#pragma once

#include "CoreMinimal.h"
#include "ShipBuilder/ShipBlueprintTypes.h"

class UShipModuleCatalog;

/**
 * Сериализация FShipBlueprintDocument в JSON и обратно.
 */
class SPACESHIPCREW_API FShipBlueprintSerializer
{
public:
	static bool DocumentToJsonString(const FShipBlueprintDocument& Document, FString& OutJson);
	static bool JsonStringToDocument(const FString& Json, FShipBlueprintDocument& OutDocument, FString& OutError);

	/** Проверяет, что все ModuleId из layout резолвятся в каталоге. */
	static bool ValidateModuleReferences(
		const FShipBlueprintDocument& Document,
		const UShipModuleCatalog& Catalog,
		TArray<FString>& OutErrors);

	/** Проверка перед записью в Saved/ShipBlueprints (модули + каталог). */
	static bool ValidateDocumentForSave(
		const FShipBlueprintDocument& Document,
		const UShipModuleCatalog& Catalog,
		FString& OutError);
};
