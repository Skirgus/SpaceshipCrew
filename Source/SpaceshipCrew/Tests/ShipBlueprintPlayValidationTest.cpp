// Автотест: ValidateDocumentForPlay для чертежа корабля.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "ShipBuilder/ShipBlueprintSerializer.h"
#include "ShipBuilder/ShipBlueprintTypes.h"
#include "ShipModule/ShipModuleCatalog.h"
#include "ShipModule/ShipModuleDefinition.h"

namespace ShipBlueprintPlayValidationTestPrivate
{
	static TArray<EShipModuleType> AllDockableTypes()
	{
		return {
			EShipModuleType::Bridge,
			EShipModuleType::Corridor,
			EShipModuleType::Reactor,
			EShipModuleType::Engine,
			EShipModuleType::FuelTank,
			EShipModuleType::OxygenTank,
			EShipModuleType::Airlock,
		};
	}

	static UShipModuleDefinition* MakeAndRegister(UShipModuleCatalog* Catalog, const FName ModuleId, const EShipModuleType Type)
	{
		UShipModuleDefinition* Definition = NewObject<UShipModuleDefinition>();
		Definition->ModuleId = ModuleId;
		Definition->ModuleType = Type;
		Definition->DisplayName = FText::FromString(ModuleId.ToString());
		Definition->Mass = 100.0f;
		Definition->Size = FVector(400.0f, 400.0f, 300.0f);
		Definition->CompatibleModuleTypes = AllDockableTypes();
		Definition->EnsureContactPointsPopulatedIfNoAuthoringOverride();
		Catalog->RegisterDefinitionForAutomation(Definition);
		return Definition;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShipBlueprintPlayValidationTest,
	"SpaceshipCrew.ShipBlueprint.ValidateDocumentForPlay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)

bool FShipBlueprintPlayValidationTest::RunTest(const FString& Parameters)
{
	using namespace ShipBlueprintPlayValidationTestPrivate;

	UShipModuleCatalog* Catalog = NewObject<UShipModuleCatalog>();
	MakeAndRegister(Catalog, FName(TEXT("DocBridge")), EShipModuleType::Bridge);
	MakeAndRegister(Catalog, FName(TEXT("DocCorridor")), EShipModuleType::Corridor);

	FShipBlueprintDocument DraftDoc;
	DraftDoc.ModuleLayout.ModuleIds = { FName(TEXT("DocBridge")), FName(TEXT("DocCorridor")) };

	TArray<FString> Blockers;
	TestFalse(TEXT("IncompleteNotPlayReady"),
		FShipBlueprintSerializer::ValidateDocumentForPlay(DraftDoc, *Catalog, Blockers));
	TestTrue(TEXT("IncompleteHasBlockers"), Blockers.Num() > 0);

	MakeAndRegister(Catalog, FName(TEXT("DocReactor")), EShipModuleType::Reactor);
	MakeAndRegister(Catalog, FName(TEXT("DocFuel")), EShipModuleType::FuelTank);
	MakeAndRegister(Catalog, FName(TEXT("DocOxygen")), EShipModuleType::OxygenTank);
	MakeAndRegister(Catalog, FName(TEXT("DocEngine")), EShipModuleType::Engine);
	MakeAndRegister(Catalog, FName(TEXT("DocAirlock")), EShipModuleType::Airlock);

	FShipBlueprintDocument CompleteDoc;
	CompleteDoc.ModuleLayout.ModuleIds = {
		FName(TEXT("DocBridge")),
		FName(TEXT("DocReactor")),
		FName(TEXT("DocFuel")),
		FName(TEXT("DocOxygen")),
		FName(TEXT("DocEngine")),
		FName(TEXT("DocAirlock")),
	};

	Blockers.Reset();
	TestTrue(TEXT("CompletePlayReady"),
		FShipBlueprintSerializer::ValidateDocumentForPlay(CompleteDoc, *Catalog, Blockers));
	TestEqual(TEXT("CompleteNoBlockers"), Blockers.Num(), 0);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
