// Copyright Epic Games, Inc. All Rights Reserved.

#include "CrewInteractionComponent.h"
#include "ShipInteractableBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

UCrewInteractionComponent::UCrewInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UCrewInteractionComponent::RequestInteract()
{
	APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn || !Pawn->IsLocallyControlled())
	{
		return;
	}
	AActor* Target = TraceInteractable();
	if (Target)
	{
		ServerTryInteract(Target);
	}
}

AActor* UCrewInteractionComponent::TraceInteractable() const
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn)
	{
		return nullptr;
	}
	AController* Controller = Pawn->GetController();
	if (!Controller)
	{
		return nullptr;
	}
	FVector ViewLoc;
	FRotator ViewRot;
	Controller->GetPlayerViewPoint(ViewLoc, ViewRot);
	const FVector End = ViewLoc + ViewRot.Vector() * TraceDistance;

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(CrewInteractTrace), false, Pawn);
	if (GetWorld()->LineTraceSingleByChannel(Hit, ViewLoc, End, ECC_Visibility, Params))
	{
		return Hit.GetActor();
	}
	return nullptr;
}

bool UCrewInteractionComponent::ValidateInteractDistance(AActor* Target) const
{
	if (!Target || !GetOwner())
	{
		return false;
	}
	const float DistSq = FVector::DistSquared(GetOwner()->GetActorLocation(), Target->GetActorLocation());
	return DistSq <= FMath::Square(TraceDistance * 1.5f);
}

bool UCrewInteractionComponent::ServerTryInteract_Validate(AActor* TargetActor)
{
	return TargetActor != nullptr;
}

void UCrewInteractionComponent::ServerTryInteract_Implementation(AActor* TargetActor)
{
	if (!TargetActor || !GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	if (!ValidateInteractDistance(TargetActor))
	{
		return;
	}
	AShipInteractableBase* Interactable = Cast<AShipInteractableBase>(TargetActor);
	if (!Interactable)
	{
		return;
	}
	APawn* Pawn = Cast<APawn>(GetOwner());
	AController* C = Pawn ? Pawn->GetController() : nullptr;
	Interactable->ExecuteInteract(C);
}
