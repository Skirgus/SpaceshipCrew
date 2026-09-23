#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EngineerTrainingScenario.generated.h"

class ADamagedHullPanel;
class AEngineerEnergyConsole;

UENUM(BlueprintType)
enum class EEngineerTrainingStep : uint8
{
	Brief UMETA(DisplayName = "Brief"),
	Energy UMETA(DisplayName = "Energy"),
	Repair UMETA(DisplayName = "Repair"),
	Complete UMETA(DisplayName = "Complete")
};

/**
 * Директор тренировки инженера: шаги Brief → Energy → Repair → Complete.
 * Слушает консоль энергосети и панель ремонта на уровне.
 */
UCLASS(Blueprintable)
class SPACESHIPCREW_API AEngineerTrainingScenario : public AActor
{
	GENERATED_BODY()

public:
	AEngineerTrainingScenario();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "Training")
	void StartScenario();

	UFUNCTION(BlueprintCallable, Category = "Training")
	void AdvanceFromBrief();

	UFUNCTION(BlueprintPure, Category = "Training")
	EEngineerTrainingStep GetCurrentStep() const { return CurrentStep; }

	UFUNCTION(BlueprintPure, Category = "Training")
	FText GetObjectiveText() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Training")
	TObjectPtr<AEngineerEnergyConsole> EnergyConsole;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Training")
	TObjectPtr<ADamagedHullPanel> DamagedPanel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Training")
	float RequiredShieldsPercent = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Training")
	bool bReturnToMenuOnComplete = true;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTrainingStepChanged, EEngineerTrainingStep, NewStep);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTrainingCompleted);

	UPROPERTY(BlueprintAssignable, Category = "Training")
	FOnTrainingStepChanged OnStepChanged;

	UPROPERTY(BlueprintAssignable, Category = "Training")
	FOnTrainingCompleted OnCompleted;

protected:
	void BindLevelActorsIfNeeded();
	void SetStep(EEngineerTrainingStep NewStep);
	void EvaluateEnergyStep();
	void EvaluateRepairStep();
	void HandleComplete();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Training")
	EEngineerTrainingStep CurrentStep = EEngineerTrainingStep::Brief;

	bool bCompletedHandled = false;
};
