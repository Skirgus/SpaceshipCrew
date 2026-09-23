#include "EngineerTrainingScenario.h"

#include "DamagedHullPanel.h"
#include "EngineerEnergyConsole.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Menu/SpaceshipCrewLevelTravel.h"
#include "ShipEnergyGrid.h"

AEngineerTrainingScenario::AEngineerTrainingScenario()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AEngineerTrainingScenario::BeginPlay()
{
	Super::BeginPlay();
	BindLevelActorsIfNeeded();
	StartScenario();
}

void AEngineerTrainingScenario::BindLevelActorsIfNeeded()
{
	if (!EnergyConsole)
	{
		EnergyConsole = Cast<AEngineerEnergyConsole>(
			UGameplayStatics::GetActorOfClass(GetWorld(), AEngineerEnergyConsole::StaticClass()));
	}
	if (!DamagedPanel)
	{
		DamagedPanel = Cast<ADamagedHullPanel>(
			UGameplayStatics::GetActorOfClass(GetWorld(), ADamagedHullPanel::StaticClass()));
	}
}

void AEngineerTrainingScenario::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			7100,
			0.0f,
			FColor::Yellow,
			GetObjectiveText().ToString());
	}

	switch (CurrentStep)
	{
	case EEngineerTrainingStep::Energy:
		EvaluateEnergyStep();
		break;
	case EEngineerTrainingStep::Repair:
		EvaluateRepairStep();
		break;
	default:
		break;
	}
}

void AEngineerTrainingScenario::StartScenario()
{
	bCompletedHandled = false;
	SetStep(EEngineerTrainingStep::Brief);
	// Авто-переход с брифа: игрок сразу получает первую задачу (UI брифа — в BP позже).
	AdvanceFromBrief();
}

void AEngineerTrainingScenario::AdvanceFromBrief()
{
	if (CurrentStep == EEngineerTrainingStep::Brief)
	{
		SetStep(EEngineerTrainingStep::Energy);
	}
}

FText AEngineerTrainingScenario::GetObjectiveText() const
{
	switch (CurrentStep)
	{
	case EEngineerTrainingStep::Brief:
		return NSLOCTEXT("SpaceshipCrew", "Train_Brief", "Учебная авария: стабилизируй корабль.");
	case EEngineerTrainingStep::Energy:
		return NSLOCTEXT("SpaceshipCrew", "Train_Energy", "Займи консоль энергосети и усиль щиты (≥ 50%) или пресет «Оборона».");
	case EEngineerTrainingStep::Repair:
		return NSLOCTEXT("SpaceshipCrew", "Train_Repair", "Отремонтируй повреждённую панель горелкой.");
	case EEngineerTrainingStep::Complete:
		return NSLOCTEXT("SpaceshipCrew", "Train_Complete", "Тренировка пройдена.");
	default:
		return FText::GetEmpty();
	}
}

void AEngineerTrainingScenario::SetStep(EEngineerTrainingStep NewStep)
{
	if (CurrentStep == NewStep)
	{
		return;
	}
	CurrentStep = NewStep;
	OnStepChanged.Broadcast(CurrentStep);
	UE_LOG(LogTemp, Log, TEXT("EngineerTraining: step -> %d"), static_cast<int32>(CurrentStep));
}

void AEngineerTrainingScenario::EvaluateEnergyStep()
{
	if (!EnergyConsole)
	{
		return;
	}
	if (UShipEnergyGrid* Grid = EnergyConsole->GetEnergyGrid())
	{
		if (Grid->MeetsShieldThreshold(RequiredShieldsPercent))
		{
			SetStep(EEngineerTrainingStep::Repair);
		}
	}
}

void AEngineerTrainingScenario::EvaluateRepairStep()
{
	if (DamagedPanel && DamagedPanel->IsFullyRepaired())
	{
		SetStep(EEngineerTrainingStep::Complete);
		HandleComplete();
	}
}

void AEngineerTrainingScenario::HandleComplete()
{
	if (bCompletedHandled)
	{
		return;
	}
	bCompletedHandled = true;
	OnCompleted.Broadcast();

	if (bReturnToMenuOnComplete)
	{
		if (UWorld* World = GetWorld())
		{
			UGameplayStatics::OpenLevel(
				World,
				FName(SpaceshipCrewLevelTravel::GetPlayMapPackagePath()),
				true,
				SpaceshipCrewLevelTravel::GetMainMenuGameOptions());
		}
	}
}
