#pragma once

#include "CoreMinimal.h"
#include "CrewInteractable.h"
#include "GameFramework/Actor.h"
#include "CrewWorkstation.generated.h"

class APawn;
class UStaticMesh;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class ECrewStationRole : uint8
{
	None UMETA(DisplayName = "None"),
	Engineer UMETA(DisplayName = "Engineer"),
	Pilot UMETA(DisplayName = "Pilot"),
	Captain UMETA(DisplayName = "Captain"),
	Gunner UMETA(DisplayName = "Gunner")
};

/**
 * Базовая рабочая станция экипажа: один оператор, Occupy / Leave через Interact.
 */
UCLASS(Blueprintable)
class SPACESHIPCREW_API ACrewWorkstation : public AActor, public ICrewInteractable
{
	GENERATED_BODY()

public:
	ACrewWorkstation();

	virtual bool CanInteract_Implementation(APawn* InstigatorPawn) const override;
	virtual void Interact_Implementation(APawn* InstigatorPawn) override;
	virtual FText GetInteractPrompt_Implementation(APawn* InstigatorPawn) const override;

	UFUNCTION(BlueprintCallable, Category = "Crew|Station")
	bool Occupy(APawn* Operator);

	UFUNCTION(BlueprintCallable, Category = "Crew|Station")
	void Leave();

	UFUNCTION(BlueprintPure, Category = "Crew|Station")
	bool IsOccupied() const { return OperatorPawn != nullptr; }

	UFUNCTION(BlueprintPure, Category = "Crew|Station")
	APawn* GetOperator() const { return OperatorPawn; }

	/** Назначить меш станции (импорт / bootstrap уровня). */
	void SetDisplayMesh(UStaticMesh* InMesh);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crew|Station")
	ECrewStationRole StationRole = ECrewStationRole::Engineer;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWorkstationOccupied, APawn*, Operator);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWorkstationLeft);

	UPROPERTY(BlueprintAssignable, Category = "Crew|Station")
	FOnWorkstationOccupied OnOccupied;

	UPROPERTY(BlueprintAssignable, Category = "Crew|Station")
	FOnWorkstationLeft OnLeft;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Crew|Station")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Crew|Station")
	TObjectPtr<USceneComponent> OccupyAnchor;

	UPROPERTY(Transient)
	TObjectPtr<APawn> OperatorPawn;
};
