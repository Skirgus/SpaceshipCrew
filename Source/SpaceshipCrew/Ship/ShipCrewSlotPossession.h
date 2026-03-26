// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ShipCrewSlotPossession.generated.h"

class APlayerController;

/** Future coop: hand off a crew slot from bot to a connected player without changing ship systems. */
UINTERFACE(MinimalAPI, Blueprintable)
class UShipCrewSlotPossession : public UInterface
{
	GENERATED_BODY()
};

class SPACESHIPCREW_API IShipCrewSlotPossession
{
	GENERATED_IINTERFACE_BODY()

public:
	virtual void PossessCrewSlot(APlayerController* Player, int32 SlotIndex);
};
