// Автотест валидации контактных точек UShipModuleDefinition:
// без записанных точек невалидно; Ensure заполняет шесть граней; дубликаты на определении ломают валидацию;
// authoring в VisualOverride.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "ShipModuleDefinition.h"
#include "ShipModuleVisualOverride.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShipModuleContactPointValidationTest,
	"SpaceshipCrew.ShipModule.ContactPointValidation",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

/** Заполняет модуль минимально валидными значениями (без authoring сокетов в override). */
static void FillBaseFields(UShipModuleDefinition* Def)
{
	Def->ModuleId = FName(TEXT("CP_Test"));
	Def->ModuleType = EShipModuleType::Corridor;
	Def->DisplayName = FText::FromString(TEXT("Тестовый коридор"));
	Def->Mass = 50.0f;
	Def->Size = FVector(400.0, 200.0, 300.0);
}

bool FShipModuleContactPointValidationTest::RunTest(const FString& Parameters)
{
	UShipModuleDefinition* Def = NewObject<UShipModuleDefinition>();
	FillBaseFields(Def);

	// --- Пустой ContactPoints и без override → невалидно (нет «виртуальных» сокетов) ---
	Def->ContactPoints.Empty();
	{
		TArray<FText> Errors;
		const bool bValid = Def->Validate(Errors);
		TestFalse(TEXT("EmptyContactPoints_ShouldFail"), bValid);
	}

	// --- После Ensure на определении — валидно и ровно шесть точек в resolved ---
	Def->EnsureContactPointsPopulatedIfNoAuthoringOverride();
	{
		TArray<FText> Errors;
		const bool bValid = Def->Validate(Errors);
		TestTrue(TEXT("AfterEnsure_ShouldPass"), bValid);

		TArray<FShipModuleContactPoint> Effective;
		Def->GatherEffectiveContactPoints(Effective);
		TestEqual(TEXT("AfterEnsure_GatherHasSix"), Effective.Num(), 6);
		TestEqual(TEXT("AfterEnsure_TopName"), Effective[4].SocketName, FName(TEXT("Top")));
		TestTrue(TEXT("AfterEnsure_TopIsVertical"),
			Effective[4].SocketType == EShipModuleSocketType::Vertical);
	}

	// --- Дубликаты имён в ContactPoints на определении → невалидно ---
	{
		Def->ContactPoints.Empty();
		FShipModuleContactPoint A;
		A.SocketName = FName(TEXT("Left"));
		A.RelativeLocation = FVector(-100.0, 0.0, 0.0);
		A.SocketType = EShipModuleSocketType::Horizontal;
		Def->ContactPoints.Add(A);
		FShipModuleContactPoint B;
		B.SocketName = FName(TEXT("Left"));
		B.RelativeLocation = FVector(0.0, 50.0, 0.0);
		B.SocketType = EShipModuleSocketType::Horizontal;
		Def->ContactPoints.Add(B);
		TArray<FText> Errors;
		TestFalse(TEXT("DuplicateOnDefinition_ShouldFail"), Def->Validate(Errors));
	}

	// --- Authoring в VisualOverride: одна валидная точка ---
	Def->ContactPoints.Empty();
	Def->EnsureContactPointsPopulatedIfNoAuthoringOverride();
	UShipModuleVisualOverride* Vis = NewObject<UShipModuleVisualOverride>(Def);
	Vis->bOverrideContactPoints = true;
	Vis->ContactPointsOverride.Reset();
	{
		FShipModuleContactPoint CP;
		CP.SocketName = FName(TEXT("CustomA"));
		CP.RelativeLocation = FVector(50.0, 0.0, 0.0);
		CP.SocketType = EShipModuleSocketType::Horizontal;
		Vis->ContactPointsOverride.Add(CP);
	}
	Def->VisualOverride = Vis;
	{
		TArray<FText> Errors;
		TestTrue(TEXT("OneOverrideCP_ShouldPass"), Def->Validate(Errors));
		TArray<FShipModuleContactPoint> Effective;
		Def->GatherEffectiveContactPoints(Effective);
		TestEqual(TEXT("OverrideReplacesDefinition"), Effective.Num(), 1);
	}

	// --- Дубликат имён в override → невалидно ---
	{
		FShipModuleContactPoint CP2;
		CP2.SocketName = FName(TEXT("CustomA"));
		CP2.RelativeLocation = FVector(0.0, 50.0, 0.0);
		CP2.SocketType = EShipModuleSocketType::Horizontal;
		Vis->ContactPointsOverride.Add(CP2);
	}
	{
		TArray<FText> Errors;
		const bool bValid = Def->Validate(Errors);
		TestFalse(TEXT("DuplicateSocketName_ShouldFail"), bValid);
		bool bFoundDupError = false;
		for (const FText& Err : Errors)
		{
			if (Err.ToString().Contains(TEXT("дублирующ")))
			{
				bFoundDupError = true;
				break;
			}
		}
		TestTrue(TEXT("DuplicateSocketName_HasRelevantError"), bFoundDupError);
	}

	// --- Убираем дубликат, пустое имя в override → невалидно ---
	Vis->ContactPointsOverride.RemoveAt(1);
	{
		FShipModuleContactPoint CP3;
		CP3.SocketName = NAME_None;
		CP3.RelativeLocation = FVector(0.0, 0.0, 20.0);
		CP3.SocketType = EShipModuleSocketType::Vertical;
		Vis->ContactPointsOverride.Add(CP3);
	}
	{
		TArray<FText> Errors;
		const bool bValid = Def->Validate(Errors);
		TestFalse(TEXT("EmptySocketName_ShouldFail"), bValid);
		bool bFoundNameError = false;
		for (const FText& Err : Errors)
		{
			if (Err.ToString().Contains(TEXT("SocketName")))
			{
				bFoundNameError = true;
				break;
			}
		}
		TestTrue(TEXT("EmptySocketName_HasRelevantError"), bFoundNameError);
	}

	return true;
}

#endif
