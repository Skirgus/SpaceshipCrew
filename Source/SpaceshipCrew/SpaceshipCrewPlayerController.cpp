// Copyright Epic Games, Inc. All Rights Reserved.


#include "SpaceshipCrewPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "SpaceshipCrew.h"
#include "Widgets/Input/SVirtualJoystick.h"

void ASpaceshipCrewPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// создаём сенсорное управление только для локальных контроллеров игрока
	if (ShouldUseTouchControls() && IsLocalPlayerController())
	{
		// создаём виджет мобильного управления
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// добавляем элементы управления на экран игрока
			MobileControlsWidget->AddToPlayerScreen(0);

		} else {

			UE_LOG(LogSpaceshipCrew, Error, TEXT("Could not spawn mobile controls widget."));

		}

	}
}

void ASpaceshipCrewPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// добавляем IMC только для локальных контроллеров игрока
	if (IsLocalPlayerController())
	{
		// Add Контексты привязки ввода
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// добавляем эти IMC только если не используется мобильный сенсорный ввод
			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}
	}
}

bool ASpaceshipCrewPlayerController::ShouldUseTouchControls() const
{
	// мы на мобильной платформе? Нужно ли принудительно включить сенсорный ввод?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}

