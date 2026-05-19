#include "ShipBlueprintSerializer.h"

#include "ShipBuilder/ShipBuilderDomainGlue.h"
#include "ShipBuildDomain.h"
#include "ShipModuleCatalog.h"
#include "ShipModuleDefinition.h"
#include "JsonObjectConverter.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonReader.h"

bool FShipBlueprintSerializer::DocumentToJsonString(const FShipBlueprintDocument& Document, FString& OutJson)
{
	const TSharedPtr<FJsonObject> RootObject = FJsonObjectConverter::UStructToJsonObject(Document);
	if (!RootObject.IsValid())
	{
		return false;
	}

	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutJson);
	return FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);
}

bool FShipBlueprintSerializer::JsonStringToDocument(const FString& Json, FShipBlueprintDocument& OutDocument, FString& OutError)
{
	TSharedPtr<FJsonObject> RootObject;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
	if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
	{
		OutError = TEXT("Некорректный JSON.");
		return false;
	}

	OutDocument = FShipBlueprintDocument();
	if (!FJsonObjectConverter::JsonObjectToUStruct(RootObject.ToSharedRef(), FShipBlueprintDocument::StaticStruct(), &OutDocument))
	{
		OutError = TEXT("Не удалось разобрать поля документа.");
		return false;
	}

	if (OutDocument.SchemaVersion <= 0)
	{
		OutDocument.SchemaVersion = 1;
	}
	if (OutDocument.SchemaVersion > FShipBlueprintDocument::CurrentSchemaVersion)
	{
		OutError = FString::Printf(
			TEXT("Неподдерживаемая версия схемы: %d (макс. %d)."),
			OutDocument.SchemaVersion,
			FShipBlueprintDocument::CurrentSchemaVersion);
		return false;
	}

	if (OutDocument.DisplayName.IsEmpty() && !OutDocument.ShipId.IsNone())
	{
		OutDocument.DisplayName = FText::FromName(OutDocument.ShipId);
	}

	return true;
}

bool FShipBlueprintSerializer::ValidateModuleReferences(
	const FShipBlueprintDocument& Document,
	const UShipModuleCatalog& Catalog,
	TArray<FString>& OutErrors)
{
	OutErrors.Reset();
	const FShipBuilderDraftConfig& Layout = Document.ModuleLayout;

	auto CheckModuleId = [&](const FName ModuleId)
	{
		if (ModuleId.IsNone())
		{
			return;
		}
		if (!Catalog.FindModuleById(ModuleId))
		{
			OutErrors.Add(FString::Printf(TEXT("Модуль не найден в каталоге: %s"), *ModuleId.ToString()));
		}
	};

	for (const FName Id : Layout.ModuleIds)
	{
		CheckModuleId(Id);
	}
	for (const FShipBuilderPlacedModule& Placed : Layout.PlacedModules)
	{
		CheckModuleId(Placed.ModuleId);
	}

	return OutErrors.Num() == 0;
}

bool FShipBlueprintSerializer::ValidateDocumentForSave(
	const FShipBlueprintDocument& Document,
	const UShipModuleCatalog& Catalog,
	FString& OutError)
{
	TArray<FString> Errors;
	if (!ValidateModuleReferences(Document, Catalog, Errors))
	{
		OutError = Errors.Num() > 0
			? FString::Join(Errors, TEXT("\n"))
			: TEXT("Неизвестные модули в чертеже.");
		return false;
	}

	if (Document.ModuleLayout.PlacedModules.Num() == 0 && Document.ModuleLayout.ModuleIds.Num() == 0)
	{
		OutError = TEXT("Нельзя сохранить пустой чертёж.");
		return false;
	}

	return true;
}

bool FShipBlueprintSerializer::ValidateDocumentForPlay(
	const FShipBlueprintDocument& Document,
	const UShipModuleCatalog& Catalog,
	TArray<FString>& OutPlayBlockers)
{
	OutPlayBlockers.Reset();

	TArray<FString> ModuleErrors;
	if (!ValidateModuleReferences(Document, Catalog, ModuleErrors))
	{
		OutPlayBlockers.Append(ModuleErrors);
		return false;
	}

	FCatalogShipBuildModuleResolver Resolver(Catalog);
	FShipBuildDomainModel Model(Resolver);
	FString BuildError;
	if (!SpaceshipCrew_BuildDomainFromDraftChain(Document.ModuleLayout, Resolver, Model, BuildError))
	{
		if (!BuildError.IsEmpty())
		{
			OutPlayBlockers.Add(BuildError);
		}
		return false;
	}

	const FShipBuildValidationResult Result = Model.Validate();
	OutPlayBlockers = Result.PlayBlockers;
	return Result.bIsPlayReady;
}
