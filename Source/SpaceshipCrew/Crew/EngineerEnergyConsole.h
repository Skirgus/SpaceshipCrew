#pragma once

#include "CoreMinimal.h"
#include "CrewWorkstation.h"
#include "EngineerEnergyConsole.generated.h"

class UShipEnergyGrid;

/**
 * Станция инженера: консоль энергосети. Хранит UShipEnergyGrid, события для UI/сценария.
 */
UCLASS(Blueprintable)
class SPACESHIPCREW_API AEngineerEnergyConsole : public ACrewWorkstation
{
	GENERATED_BODY()

public:
	AEngineerEnergyConsole();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintPure, Category = "Crew|Energy")
	UShipEnergyGrid* GetEnergyGrid() const { return EnergyGrid; }

	UFUNCTION(BlueprintCallable, Category = "Crew|Energy")
	void SetEnergyShares(float Shields, float Weapons, float Engines);

	UFUNCTION(BlueprintCallable, Category = "Crew|Energy")
	void ApplyDefensePreset();

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEnergySharesChanged);

	UPROPERTY(BlueprintAssignable, Category = "Crew|Energy")
	FOnEnergySharesChanged OnEnergySharesChanged;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Crew|Energy")
	TObjectPtr<UShipEnergyGrid> EnergyGrid;
};
