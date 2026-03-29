// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShipCrewCharacter.h"
#include "CrewBotBrainComponent.h"
#include "CrewInteractionComponent.h"
#include "CrewRoleComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "SpaceshipCrew.h"
#include "TimerManager.h"

AShipCrewCharacter::AShipCrewCharacter()
{
	bReplicates = true;
	SetReplicateMovement(true);

	CrewRole = CreateDefaultSubobject<UCrewRoleComponent>(TEXT("CrewRole"));
	CrewInteraction = CreateDefaultSubobject<UCrewInteractionComponent>(TEXT("CrewInteraction"));
	CrewBotBrain = CreateDefaultSubobject<UCrewBotBrainComponent>(TEXT("CrewBotBrain"));
}

void AShipCrewCharacter::BeginPlay()
{
	Super::BeginPlay();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &AShipCrewCharacter::BindInteractFallbackNextTick));
	}
}

void AShipCrewCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	// Здесь PlayerInputComponent уже создан; у пешки поле InputComponent может ещё не совпадать с ним до конца вызова.
	TryBindInteractInput(PlayerInputComponent);
}

void AShipCrewCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	// Ввод: задайте в BP Auto Receive Input = Player 0 (или вызывайте EnableInput только когда PC == GetController()).
	TryBindInteractInput(InputComponent);
}

void AShipCrewCharacter::UnPossessed()
{
	bInteractInputBound = false;
	Super::UnPossessed();
}

void AShipCrewCharacter::BindInteractFallbackNextTick()
{
	if (!bInteractInputBound)
	{
		TryBindInteractInput(InputComponent);
	}
}

void AShipCrewCharacter::TryBindInteractInput(UInputComponent* IC)
{
	if (!IsLocallyControlled())
	{
		return;
	}
	if (!InteractAction || !CrewInteraction)
	{
		return;
	}
	if (bInteractInputBound)
	{
		return;
	}
	UInputComponent* UseIC = IC ? IC : InputComponent.Get();
	if (!UseIC)
	{
		return;
	}
	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(UseIC))
	{
		// Started иногда не приходит (тип IA / IMC); Triggered — как SideScrollingInteract.
		EIC->BindAction(InteractAction, ETriggerEvent::Triggered, this, &AShipCrewCharacter::OnInteractPressed);
		bInteractInputBound = true;
	}
}

void AShipCrewCharacter::OnInteractPressed(const FInputActionValue& Value)
{
	if (Value.GetValueType() == EInputActionValueType::Boolean && !Value.Get<bool>())
	{
		return;
	}
	if (CrewInteraction)
	{
		CrewInteraction->RequestInteract();
	}
}
