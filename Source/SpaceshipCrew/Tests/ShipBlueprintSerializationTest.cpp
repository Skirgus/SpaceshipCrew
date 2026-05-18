// Ship blueprint serialization and policy automation tests (T03).

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "ShipBuilder/ShipBlueprintNaming.h"
#include "ShipBuilder/ShipBlueprintSerializer.h"
#include "ShipBuilder/ShipBlueprintSessionSubsystem.h"
#include "ShipBuilder/ShipBlueprintTypes.h"
#include "ShipModule/ShipModuleCatalog.h"
#include "ShipModule/ShipModuleDefinition.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShipBlueprintJsonRoundTripTest,
	"SpaceshipCrew.ShipBlueprint.JsonRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)

bool FShipBlueprintJsonRoundTripTest::RunTest(const FString& Parameters)
{
	FShipBlueprintDocument Original;
	Original.SchemaVersion = FShipBlueprintDocument::CurrentSchemaVersion;
	Original.ShipId = FName(TEXT("TestShip"));
	Original.DisplayName = FText::FromString(TEXT("Test Ship"));

	FShipBuilderPlacedModule ModuleA;
	ModuleA.InstanceId = FName(TEXT("InstA"));
	ModuleA.ModuleId = FName(TEXT("TestBridge"));
	ModuleA.GridPos = FIntVector(0, 0, 0);
	Original.ModuleLayout.PlacedModules.Add(ModuleA);
	Original.ModuleLayout.ModuleIds.Add(ModuleA.ModuleId);

	FString Json;
	TestTrue(TEXT("Serialize"), FShipBlueprintSerializer::DocumentToJsonString(Original, Json));
	TestFalse(TEXT("JsonNotEmpty"), Json.IsEmpty());

	FShipBlueprintDocument Loaded;
	FString Error;
	TestTrue(TEXT("Deserialize"), FShipBlueprintSerializer::JsonStringToDocument(Json, Loaded, Error));
	TestEqual(TEXT("ShipId"), Loaded.ShipId, Original.ShipId);
	TestEqual(TEXT("PlacedCount"), Loaded.ModuleLayout.PlacedModules.Num(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShipBlueprintSessionSavePolicyTest,
	"SpaceshipCrew.ShipBlueprint.SessionSavePolicy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)

bool FShipBlueprintSessionSavePolicyTest::RunTest(const FString& Parameters)
{
	UShipBlueprintSessionSubsystem* Session = NewObject<UShipBlueprintSessionSubsystem>();

	Session->GetSession().EditSource = EShipBlueprintEditSource::ProjectTemplate;
	Session->GetSession().bRequiresSaveAsOnWrite = true;
	TestFalse(TEXT("ProjectTemplateCannotSaveInPlace"), Session->CanSaveInPlace());

	FString SaveError;
	TestFalse(TEXT("ProjectTemplateSaveCurrentFails"), Session->SaveCurrent(SaveError));
	TestFalse(TEXT("SaveErrorNotEmpty"), SaveError.IsEmpty());

	Session->GetSession().EditSource = EShipBlueprintEditSource::PlayerOwned;
	Session->GetSession().bRequiresSaveAsOnWrite = false;
	Session->GetSession().SourceShipId = FName(TEXT("player_ship_abc12345"));
	TestTrue(TEXT("PlayerOwnedCanSaveInPlace"), Session->CanSaveInPlace());

	Session->GetSession().EditSource = EShipBlueprintEditSource::New;
	Session->GetSession().bRequiresSaveAsOnWrite = false;
	Session->GetSession().SourceShipId = NAME_None;
	TestFalse(TEXT("NewWithoutIdCannotSaveInPlace"), Session->CanSaveInPlace());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShipBlueprintModuleValidationTest,
	"SpaceshipCrew.ShipBlueprint.ValidateModuleReferences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)

bool FShipBlueprintModuleValidationTest::RunTest(const FString& Parameters)
{
	UShipModuleCatalog* Catalog = NewObject<UShipModuleCatalog>();
	UShipModuleDefinition* Known = NewObject<UShipModuleDefinition>();
	Known->ModuleId = FName(TEXT("KnownModule"));
	Known->DisplayName = FText::FromString(TEXT("Known"));
	Known->Mass = 100.0f;
	Known->Size = FVector(400.0f, 400.0f, 300.0f);
	Catalog->RegisterDefinitionForAutomation(Known);

	FShipBlueprintDocument ValidDoc;
	FShipBuilderPlacedModule Placed;
	Placed.ModuleId = FName(TEXT("KnownModule"));
	ValidDoc.ModuleLayout.PlacedModules.Add(Placed);

	TArray<FString> Errors;
	TestTrue(TEXT("KnownModuleValid"), FShipBlueprintSerializer::ValidateModuleReferences(ValidDoc, *Catalog, Errors));
	TestEqual(TEXT("NoErrors"), Errors.Num(), 0);

	FShipBlueprintDocument InvalidDoc;
	FShipBuilderPlacedModule Missing;
	Missing.ModuleId = FName(TEXT("MissingModule_XYZ"));
	InvalidDoc.ModuleLayout.PlacedModules.Add(Missing);

	Errors.Reset();
	TestFalse(TEXT("MissingModuleInvalid"), FShipBlueprintSerializer::ValidateModuleReferences(InvalidDoc, *Catalog, Errors));
	TestTrue(TEXT("HasErrors"), Errors.Num() > 0);

	const FString Json = TEXT("{\"SchemaVersion\":999,\"ShipId\":\"X\",\"ModuleLayout\":{}}");
	FShipBlueprintDocument Loaded;
	FString ParseError;
	TestFalse(TEXT("RejectFutureSchema"), FShipBlueprintSerializer::JsonStringToDocument(Json, Loaded, ParseError));

	FShipBlueprintDocument EmptyDoc;
	FString SaveError;
	TestFalse(TEXT("EmptyLayoutRejected"),
		FShipBlueprintSerializer::ValidateDocumentForSave(EmptyDoc, *Catalog, SaveError));
	TestTrue(TEXT("NonemptyLayoutAccepted"),
		FShipBlueprintSerializer::ValidateDocumentForSave(ValidDoc, *Catalog, SaveError));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShipBlueprintNamingCyrillicTest,
	"SpaceshipCrew.ShipBlueprint.NamingCyrillicDistinct",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)

bool FShipBlueprintNamingCyrillicTest::RunTest(const FString& Parameters)
{
	const FName IdA = ShipBlueprintNaming::MakeShipIdFromDisplayName(TEXT("Мой корабль"));
	const FName IdB = ShipBlueprintNaming::MakeShipIdFromDisplayName(TEXT("Стартовый шаблон"));

	TestFalse(TEXT("IdsDistinct"), IdA == IdB);
	TestFalse(TEXT("IdANotBareShip"), IdA.ToString().Equals(TEXT("ship"), ESearchCase::IgnoreCase));
	TestTrue(TEXT("IdAHasSlug"), IdA.ToString().Contains(TEXT("moy")));
	TestTrue(TEXT("IdBHasSlug"), IdB.ToString().Contains(TEXT("startov")));

	const FName IdA2 = ShipBlueprintNaming::MakeShipIdFromDisplayName(TEXT("Мой корабль"));
	TestEqual(TEXT("StableHash"), IdA, IdA2);

	FString SafeName;
	TestTrue(TEXT("ValidIdSanitized"), ShipBlueprintNaming::TrySanitizeShipIdForFilename(IdA, SafeName));
	TestFalse(TEXT("PathTraversalRejected"), ShipBlueprintNaming::TrySanitizeShipIdForFilename(FName(TEXT("../evil")), SafeName));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
