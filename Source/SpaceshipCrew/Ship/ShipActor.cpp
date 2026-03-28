// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShipActor.h"
#include "ShipSystemsComponent.h"
#include "Components/StaticMeshComponent.h"

AShipActor::AShipActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(false);

	HullMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HullMesh"));
	RootComponent = HullMesh;

	ShipSystems = CreateDefaultSubobject<UShipSystemsComponent>(TEXT("ShipSystems"));
}
