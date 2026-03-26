// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShipInteractableBase.h"
#include "ShipActor.h"
#include "ShipSystemsComponent.h"
#include "Components/SceneComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"

AShipInteractableBase::AShipInteractableBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(false);

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(RootScene);
}

UShipSystemsComponent* AShipInteractableBase::ResolveShipSystems() const
{
	if (OwningShipActor)
	{
		return OwningShipActor->FindComponentByClass<UShipSystemsComponent>();
	}
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AShipActor> It(World); It; ++It)
		{
			if (UShipSystemsComponent* Sys = It->FindComponentByClass<UShipSystemsComponent>())
			{
				return Sys;
			}
		}
	}
	return nullptr;
}

void AShipInteractableBase::ExecuteInteract(AController* Issuer)
{
	if (!HasAuthority())
	{
		return;
	}
	UShipSystemsComponent* Sys = ResolveShipSystems();
	if (!Sys)
	{
		return;
	}
	Sys->ApplyAuthorizedAction(Issuer, RequiredPermission, ActionId, Magnitude);
}
