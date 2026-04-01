// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShipDoorActor.h"
#include "Components/StaticMeshComponent.h"

AShipDoorActor::AShipDoorActor()
{
	PrimaryActorTick.bCanEverTick = false;
	SetReplicates(false);

	DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
	RootComponent = DoorMesh;
	DoorMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	DoorMesh->SetCollisionResponseToAllChannels(ECR_Block);
	DoorMesh->SetCanEverAffectNavigation(true);
}

void AShipDoorActor::SetDoorOpen(bool bOpen)
{
	DoorMesh->SetCollisionEnabled(bOpen ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
	DoorMesh->SetVisibility(!bOpen, true);
}

