// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShipCrewCharacter.h"
#include "CrewBotBrainComponent.h"
#include "CrewInteractionComponent.h"
#include "CrewRoleComponent.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "SpaceshipCrew.h"

AShipCrewCharacter::AShipCrewCharacter()
{
	bReplicates = true;
	SetReplicateMovement(true);

	CrewRole = CreateDefaultSubobject<UCrewRoleComponent>(TEXT("CrewRole"));
	CrewInteraction = CreateDefaultSubobject<UCrewInteractionComponent>(TEXT("CrewInteraction"));
	CrewBotBrain = CreateDefaultSubobject<UCrewBotBrainComponent>(TEXT("CrewBotBrain"));
}

void AShipCrewCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (InteractAction && CrewInteraction)
	{
		if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
		{
			EIC->BindAction(InteractAction, ETriggerEvent::Started, this, &AShipCrewCharacter::OnInteractPressed);
		}
	}
}

void AShipCrewCharacter::OnInteractPressed(const FInputActionValue& Value)
{
	if (CrewInteraction)
	{
		CrewInteraction->RequestInteract();
	}
}
