// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShipCrewSlotPossession.h"

void IShipCrewSlotPossession::PossessCrewSlot(APlayerController* Player, int32 SlotIndex)
{
	// Default no-op; override on GameMode when adding networking.
}
