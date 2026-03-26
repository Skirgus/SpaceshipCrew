// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "CrewTypes.h"
#include "ShipGameState.generated.h"

/**
 * Replicated crew roster for UI and future lobby / coop handoff.
 * Ship simulation state lives on UShipSystemsComponent (replicated there).
 */
UCLASS()
class SPACESHIPCREW_API AShipGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AShipGameState();

	UPROPERTY(ReplicatedUsing = OnRep_CrewSlots, BlueprintReadOnly, Category = "Crew")
	TArray<FCrewSlotReplicationData> CrewSlots;

	/** Call from server / GameMode only. */
	void SetCrewSlotsFromAuthority(const TArray<FCrewSlotReplicationData>& InSlots);

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_CrewSlots();
};
