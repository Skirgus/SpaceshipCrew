// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShipGameState.h"
#include "Net/UnrealNetwork.h"

AShipGameState::AShipGameState()
{
	bReplicates = true;
}

void AShipGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AShipGameState, CrewSlots);
}

void AShipGameState::SetCrewSlotsFromAuthority(const TArray<FCrewSlotReplicationData>& InSlots)
{
	if (HasAuthority())
	{
		CrewSlots = InSlots;
	}
}

void AShipGameState::OnRep_CrewSlots()
{
}
