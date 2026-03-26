// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShipActor.h"
#include "ShipSystemsComponent.h"

AShipActor::AShipActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(false);

	ShipSystems = CreateDefaultSubobject<UShipSystemsComponent>(TEXT("ShipSystems"));
}
