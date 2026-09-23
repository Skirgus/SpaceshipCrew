#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Crew/DamagedHullPanel.h"
#include "Crew/EngineerTrainingScenario.h"
#include "Crew/ShipEnergyGrid.h"
#include "Engine/Engine.h"

// Только UObject: SmokeFilter крутится в FEngineLoop::PreInit до GEngine->Init().
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShipEnergyGridNormalizeTest,
	"SpaceshipCrew.Crew.EnergyGrid.NormalizeAndDefensePreset",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FShipEnergyGridNormalizeTest::RunTest(const FString& Parameters)
{
	UShipEnergyGrid* Grid = NewObject<UShipEnergyGrid>();
	Grid->SetShares(10.0f, 10.0f, 0.0f);
	const FShipEnergyShares After = Grid->GetShares();
	const float Sum = After.Shields + After.Weapons + After.Engines;
	TestTrue(TEXT("SharesSumApprox100"), FMath::IsNearlyEqual(Sum, 100.0f, 0.05f));

	Grid->ApplyDefensePreset();
	TestTrue(TEXT("DefenseShieldsAtLeast50"), Grid->MeetsShieldThreshold(50.0f));
	TestFalse(TEXT("DefenseShieldsNot90"), Grid->MeetsShieldThreshold(90.0f));
	return true;
}

// Не SmokeFilter: NewObject<AActor> на старте редактора даёт ensure
// (LogOutputDevice / пустой Script Stack) и стоп в отладчике Rider («Variable is not available»).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDamagedPanelRepairApiTest,
	"SpaceshipCrew.Crew.DamagedPanel.RepairApi",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FDamagedPanelRepairApiTest::RunTest(const FString& Parameters)
{
	if (!GEngine)
	{
		AddWarning(TEXT("Skip: GEngine is null"));
		return true;
	}

	ADamagedHullPanel* Panel = NewObject<ADamagedHullPanel>();
	Panel->SetRepairState(EHullPanelRepairState::Damaged);
	TestFalse(TEXT("NotRepairedYet"), Panel->IsFullyRepaired());
	Panel->SetRepairState(EHullPanelRepairState::Repaired);
	TestTrue(TEXT("Repaired"), Panel->IsFullyRepaired());
	TestEqual(TEXT("Progress1"), Panel->GetRepairProgress(), 1.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEngineerTrainingStepMachineTest,
	"SpaceshipCrew.Crew.EngineerTraining.StepMachine",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FEngineerTrainingStepMachineTest::RunTest(const FString& Parameters)
{
	if (!GEngine)
	{
		AddWarning(TEXT("Skip: GEngine is null"));
		return true;
	}

	AEngineerTrainingScenario* Scenario = NewObject<AEngineerTrainingScenario>();
	Scenario->bReturnToMenuOnComplete = false;
	TestEqual(TEXT("InitialBrief"), static_cast<int32>(Scenario->GetCurrentStep()), static_cast<int32>(EEngineerTrainingStep::Brief));
	Scenario->AdvanceFromBrief();
	TestEqual(TEXT("AfterBriefEnergy"), static_cast<int32>(Scenario->GetCurrentStep()), static_cast<int32>(EEngineerTrainingStep::Energy));
	TestFalse(TEXT("ObjectiveNotEmpty"), Scenario->GetObjectiveText().IsEmpty());
	return true;
}

#endif
