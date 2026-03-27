// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ShipCrewSlotPossession.generated.h"

class APlayerController;

/** Для будущего коопа: передача слота экипажа от бота подключившемуся игроку без изменений систем корабля. */
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

