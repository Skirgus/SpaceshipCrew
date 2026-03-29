// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShipReactorStation.h"

AShipReactorStation::AShipReactorStation()
{
	RequiredPermission = FName(TEXT("Reactor"));
	ActionId = FName(TEXT("AdjustReactor"));
	Magnitude = 1.f;
}
