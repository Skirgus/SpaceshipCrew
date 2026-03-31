// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShipInteractableBase.h"
#include "ShipActor.h"
#include "ShipSystemsComponent.h"
#include "SpaceshipCrew.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"

AShipInteractableBase::AShipInteractableBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(false);

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(RootScene);

	InteractionCollider = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionCollider"));
	InteractionCollider->SetupAttachment(RootScene);
	InteractionCollider->SetSphereRadius(60.f);
	InteractionCollider->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionCollider->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionCollider->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	InteractionCollider->SetGenerateOverlapEvents(false);
}

UShipSystemsComponent* AShipInteractableBase::ResolveShipSystems() const
{
	if (OwningShipActor)
	{
		if (UShipSystemsComponent* Sys = OwningShipActor->FindComponentByClass<UShipSystemsComponent>())
		{
			return Sys;
		}
		UE_LOG(LogSpaceshipCrew, Warning, TEXT("ResolveShipSystems: OwningShipActor «%s» не содержит UShipSystemsComponent — ищем корабль в мире."),
			*GetNameSafe(OwningShipActor));
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

void AShipInteractableBase::ExecuteInteract(AController* Issuer, APawn* InstigatorPawn)
{
	if (!HasAuthority())
	{
		return;
	}
	if (ActionId.IsNone())
	{
		UE_LOG(LogSpaceshipCrew, Error, TEXT("ExecuteInteract: ActionId пустой на %s — задайте в BP (для реактора: AdjustReactor)."),
			*GetNameSafe(this));
		return;
	}
	UShipSystemsComponent* Sys = ResolveShipSystems();
	if (!Sys)
	{
		UE_LOG(LogSpaceshipCrew, Warning, TEXT("ExecuteInteract: нет ShipSystems (%s)."), *GetNameSafe(this));
		return;
	}
	if (!Sys->ApplyAuthorizedAction(Issuer, RequiredPermission, ActionId, Magnitude, InstigatorPawn))
	{
		UE_LOG(LogSpaceshipCrew, Warning, TEXT("ExecuteInteract: отклонено (права или ActionId). Station=%s Permission=%s ActionId=%s"),
			*GetNameSafe(this), *RequiredPermission.ToString(), *ActionId.ToString());
	}
}
