// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ShipSystemsComponent.generated.h"

class UCrewRoleComponent;
class AController;

/**
 * Server-authoritative ship simulation. Commands must pass role authorization — no player/bot branching.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class SPACESHIPCREW_API UShipSystemsComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UShipSystemsComponent();

	/** 0–1 normalized power output. */
	UPROPERTY(ReplicatedUsing = OnRep_Snapshot, BlueprintReadOnly, Category = "Ship|Power")
	float ReactorOutput = 0.75f;

	/** 0–1 oxygen level (room aggregate MVP). */
	UPROPERTY(ReplicatedUsing = OnRep_Snapshot, BlueprintReadOnly, Category = "Ship|Atmosphere")
	float OxygenLevel = 1.f;

	/** 0–1 hull integrity. */
	UPROPERTY(ReplicatedUsing = OnRep_Snapshot, BlueprintReadOnly, Category = "Ship|Damage")
	float HullIntegrity = 1.f;

	/** 0–1 active fire intensity. */
	UPROPERTY(ReplicatedUsing = OnRep_Snapshot, BlueprintReadOnly, Category = "Ship|Fire")
	float FireIntensity = 0.f;

	/** Validates issuer's role vs StationPermission, then applies ActionId (extensible). */
	UFUNCTION(BlueprintCallable, Category = "Ship")
	bool ApplyAuthorizedAction(AController* Issuer, FName StationPermission, FName ActionId, float Magnitude = 1.f);

	UFUNCTION(BlueprintPure, Category = "Ship")
	bool CanIssuerUseStation(AController* Issuer, FName StationPermission) const;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_Snapshot();

	void SimulateSystems(float DeltaTime);

	bool InternalApplyAction(AController* Issuer, FName StationPermission, FName ActionId, float Magnitude);

	static bool ResolveCrewRole(AController* Issuer, UCrewRoleComponent*& OutRole);
};
