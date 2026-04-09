// Автотест валидации контактных точек UShipModuleDefinition:
// пустой массив, дубликаты SocketName, отсутствие имени, валидная конфигурация.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "ShipModuleDefinition.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShipModuleContactPointValidationTest,
	"SpaceshipCrew.ShipModule.ContactPointValidation",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

/** Заполняет модуль минимально валидными значениями (без контактных точек). */
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

	// --- Пустой массив контактных точек → невалидно ---
	Def->ContactPoints.Empty();
	{
		TArray<FText> Errors;
		const bool bValid = Def->Validate(Errors);
		TestFalse(TEXT("EmptyContactPoints_ShouldFail"), bValid);

		bool bFoundCPError = false;
		for (const FText& Err : Errors)
		{
			if (Err.ToString().Contains(TEXT("контактн")))
			{
				bFoundCPError = true;
				break;
			}
		}
		TestTrue(TEXT("EmptyContactPoints_HasRelevantError"), bFoundCPError);
	}

	// --- Одна валидная точка → валидно ---
	{
		FShipModuleContactPoint CP;
		CP.SocketName = FName(TEXT("Left"));
		CP.RelativeLocation = FVector(-100.0, 0.0, 0.0);
		CP.SocketType = EShipModuleSocketType::Horizontal;
		Def->ContactPoints.Add(CP);
	}
	{
		TArray<FText> Errors;
		TestTrue(TEXT("OneValidCP_ShouldPass"), Def->Validate(Errors));
	}

	// --- Две точки с уникальными именами → валидно ---
	{
		FShipModuleContactPoint CP2;
		CP2.SocketName = FName(TEXT("Right"));
		CP2.RelativeLocation = FVector(100.0, 0.0, 0.0);
		CP2.SocketType = EShipModuleSocketType::Horizontal;
		Def->ContactPoints.Add(CP2);
	}
	{
		TArray<FText> Errors;
		TestTrue(TEXT("TwoUniqueCP_ShouldPass"), Def->Validate(Errors));
	}

	// --- Дублирующийся SocketName → невалидно ---
	{
		FShipModuleContactPoint CP3;
		CP3.SocketName = FName(TEXT("Left"));
		CP3.RelativeLocation = FVector(0.0, 100.0, 0.0);
		CP3.SocketType = EShipModuleSocketType::Vertical;
		Def->ContactPoints.Add(CP3);
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

	// --- Убираем дубликат, добавляем точку с пустым именем → невалидно ---
	Def->ContactPoints.RemoveAt(2);
	{
		FShipModuleContactPoint CP4;
		CP4.SocketName = NAME_None;
		CP4.RelativeLocation = FVector(0.0, 0.0, 150.0);
		CP4.SocketType = EShipModuleSocketType::Vertical;
		Def->ContactPoints.Add(CP4);
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
