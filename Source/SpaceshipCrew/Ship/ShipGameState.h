// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "CrewTypes.h"
#include "ShipGameState.generated.h"

/**
 * Реплицируемый состав экипажа для UI и будущей передачи слотов в лобби/коопе.
 * Состояние симуляции корабля хранится в UShipSystemsComponent (и там же реплицируется).
 */
UCLASS()
class SPACESHIPCREW_API AShipGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AShipGameState();

	UPROPERTY(ReplicatedUsing = OnRep_CrewSlots, BlueprintReadOnly, Category = "Crew")
	TArray<FCrewSlotReplicationData> CrewSlots;

	/** Вызывать только с сервера / из GameMode. */
	void SetCrewSlotsFromAuthority(const TArray<FCrewSlotReplicationData>& InSlots);

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_CrewSlots();
};

