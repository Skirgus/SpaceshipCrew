// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShipCrewSlotPossession.h"

void IShipCrewSlotPossession::PossessCrewSlot(APlayerController* Player, int32 SlotIndex)
{
	// По умолчанию ничего не делает; переопределяется в GameMode при добавлении сетевой игры.
}

