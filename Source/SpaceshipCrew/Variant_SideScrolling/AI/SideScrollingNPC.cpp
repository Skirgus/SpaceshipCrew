// Copyright Epic Games, Inc. All Rights Reserved.


#include "SideScrollingNPC.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"

ASideScrollingNPC::ASideScrollingNPC()
{
 	PrimaryActorTick.bCanEverTick = true;

	GetCharacterMovement()->MaxWalkSpeed = 150.0f;
}

void ASideScrollingNPC::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// очистить deactivation таймер
	GetWorld()->GetTimerManager().ClearTimer(DeactivationTimer);
}

void ASideScrollingNPC::Interaction(AActor* Interactor)
{
	// игнорировать если этот NPC has already been deactivated
	if (bDeactivated)
	{
		return;
	}

	// сбросить deactivation flag
	bDeactivated = true;

	// stop персонаж движение immediately
	GetCharacterMovement()->StopMovementImmediately();

	// launch NPC away из interactor
	FVector LaunchVector = Interactor->GetActorForwardVector() * LaunchImpulse;
	LaunchVector.Y = 0.0f;
	LaunchVector.Z = LaunchVerticalImpulse;

	LaunchCharacter(LaunchVector, true, true);

	// установить up a таймер для schedule reactivation
	GetWorld()->GetTimerManager().SetTimer(DeactivationTimer, this, &ASideScrollingNPC::ResetDeactivation, DeactivationTime, false);
}

void ASideScrollingNPC::ResetDeactivation()
{
	// сбросить deactivation flag
	bDeactivated = false;
}




