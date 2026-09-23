#pragma once

#include "CoreMinimal.h"
#include "CrewInteractable.h"
#include "GameFramework/Actor.h"
#include "DamagedHullPanel.generated.h"

class UStaticMesh;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class EHullPanelRepairState : uint8
{
	Intact UMETA(DisplayName = "Intact"),
	Damaged UMETA(DisplayName = "Damaged"),
	Repaired UMETA(DisplayName = "Repaired")
};

/**
 * Корпусная панель с ремонтом hold-Interact (горелка проверяется снаружи / флагом bRequireTorch).
 */
UCLASS(Blueprintable)
class SPACESHIPCREW_API ADamagedHullPanel : public AActor, public ICrewInteractable
{
	GENERATED_BODY()

public:
	ADamagedHullPanel();

	virtual void Tick(float DeltaSeconds) override;

	virtual bool CanInteract_Implementation(APawn* InstigatorPawn) const override;
	virtual void Interact_Implementation(APawn* InstigatorPawn) override;
	virtual FText GetInteractPrompt_Implementation(APawn* InstigatorPawn) const override;

	UFUNCTION(BlueprintCallable, Category = "Crew|Repair")
	void BeginRepair(APawn* InstigatorPawn);

	UFUNCTION(BlueprintCallable, Category = "Crew|Repair")
	void StopRepair();

	UFUNCTION(BlueprintCallable, Category = "Crew|Repair")
	void SetRepairState(EHullPanelRepairState NewState);

	UFUNCTION(BlueprintPure, Category = "Crew|Repair")
	EHullPanelRepairState GetRepairState() const { return RepairState; }

	UFUNCTION(BlueprintPure, Category = "Crew|Repair")
	float GetRepairProgress() const { return RepairProgress; }

	UFUNCTION(BlueprintPure, Category = "Crew|Repair")
	bool IsFullyRepaired() const { return RepairState == EHullPanelRepairState::Repaired; }

	void SetDisplayMesh(UStaticMesh* InMesh);

	/** Секунды удержания Interact до 100% (при Damaged). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crew|Repair")
	float RepairDurationSeconds = 2.5f;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPanelRepaired);

	UPROPERTY(BlueprintAssignable, Category = "Crew|Repair")
	FOnPanelRepaired OnPanelRepaired;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Crew|Repair")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crew|Repair")
	EHullPanelRepairState RepairState = EHullPanelRepairState::Damaged;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Crew|Repair")
	float RepairProgress = 0.0f;

	UPROPERTY(Transient)
	TWeakObjectPtr<APawn> RepairingPawn;

	bool bRepairHolding = false;
};
