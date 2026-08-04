// Автотест валидации обязательных полей UShipModuleDefinition.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "ShipModuleDefinition.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShipModuleRequiredFieldsTest,
	"SpaceshipCrew.ShipModule.RequiredFields",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FShipModuleRequiredFieldsTest::RunTest(const FString& Parameters)
{
	UShipModuleDefinition* Def = NewObject<UShipModuleDefinition>();

	// --- Полностью пустой модуль не проходит валидацию ---
	{
		TArray<FText> Errors;
		const bool bValid = Def->Validate(Errors);
		TestFalse(TEXT("EmptyModule_ShouldFail"), bValid);
		TestTrue(TEXT("EmptyModule_HasErrors"), Errors.Num() > 0);
	}

	// --- Заполняем обязательные поля минимально корректно ---
	Def->ModuleId = FName(TEXT("TestBridge"));
	Def->ModuleType = EShipModuleType::Bridge;
	Def->DisplayName = FText::FromString(TEXT("Тестовый мостик"));
	Def->Mass = 500.0f;
	Def->Size = FVector(800.0, 600.0, 400.0);
	Def->EnsureContactPointsPopulatedIfNoAuthoringOverride();

	{
		TArray<FText> Errors;
		const bool bValid = Def->Validate(Errors);
		TestTrue(TEXT("ValidModule_ShouldPass"), bValid);
		TestEqual(TEXT("ValidModule_NoErrors"), Errors.Num(), 0);
	}

	// --- ModuleId = None → невалидно ---
	Def->ModuleId = NAME_None;
	{
		TArray<FText> Errors;
		TestFalse(TEXT("NoModuleId_ShouldFail"), Def->Validate(Errors));
	}
	Def->ModuleId = FName(TEXT("TestBridge"));

	// --- DisplayName пустой → невалидно ---
	Def->DisplayName = FText::GetEmpty();
	{
		TArray<FText> Errors;
		TestFalse(TEXT("EmptyDisplayName_ShouldFail"), Def->Validate(Errors));
	}
	Def->DisplayName = FText::FromString(TEXT("Тестовый мостик"));

	// --- Mass <= 0 → невалидно ---
	Def->Mass = 0.0f;
	{
		TArray<FText> Errors;
		TestFalse(TEXT("ZeroMass_ShouldFail"), Def->Validate(Errors));
	}
	Def->Mass = -10.0f;
	{
		TArray<FText> Errors;
		TestFalse(TEXT("NegativeMass_ShouldFail"), Def->Validate(Errors));
	}
	Def->Mass = 500.0f;

	// --- Size с нулевым компонентом → невалидно ---
	Def->Size = FVector(0.0, 600.0, 400.0);
	{
		TArray<FText> Errors;
		TestFalse(TEXT("ZeroSizeX_ShouldFail"), Def->Validate(Errors));
	}
	Def->Size = FVector(800.0, 600.0, 400.0);

	// --- Проверяем GetPrimaryAssetId ---
	{
		const FPrimaryAssetId Id = Def->GetPrimaryAssetId();
		TestEqual(TEXT("PrimaryAssetType"), Id.PrimaryAssetType.ToString(), FString(TEXT("ShipModule")));
		TestEqual(TEXT("PrimaryAssetName"), Id.PrimaryAssetName.ToString(), FString(TEXT("TestBridge")));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShipModuleCellSizeSyncTest,
	"SpaceshipCrew.ShipModule.CellSizeSync",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FShipModuleCellSizeSyncTest::RunTest(const FString& Parameters)
{
	UShipModuleDefinition* Def = NewObject<UShipModuleDefinition>();
	Def->CellSize = FIntVector(1, 1, 1);
	Def->Size = FVector(400.0f, 400.0f, 300.0f);
	UShipModuleDefinition::AppendDefaultPanelContactPointsForCellSize(Def->CellSize, Def->ContactPoints);
	TestEqual(TEXT("OneByOnePanelCount"), Def->ContactPoints.Num(), 6);

	Def->CellSize = FIntVector(1, 3, 1);
	Def->SyncCellSizeAndSizeFromLegacy();
	TestEqual(TEXT("OneByThreeSizeY"), static_cast<float>(Def->Size.Y), 1200.0f);
	TestEqual(TEXT("OneByThreeSizeX"), static_cast<float>(Def->Size.X), 400.0f);

	Def->RegenerateDefaultContactPointsFromCellSize();
	TestEqual(TEXT("OneByThreePanelCount"), Def->ContactPoints.Num(), 14);

	return true;
}

#endif
