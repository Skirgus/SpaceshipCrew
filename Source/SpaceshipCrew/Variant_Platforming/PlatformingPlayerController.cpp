// Copyright Epic Games, Inc. All Rights Reserved.


#include "Variant_Platforming/PlatformingPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "PlatformingCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Blueprint/UserWidget.h"
#include "SpaceshipCrew.h"
#include "Widgets/Input/SVirtualJoystick.h"

void APlatformingPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// только спавн сенсорное управление на локальный игрок контроллерs
	if (ShouldUseTouchControls() && IsLocalPlayerController())
	{
 // спавн мобильных controls виджет
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
 // add controls для игрок screen
			MobileControlsWidget->AddToPlayerScreen(0);

		} else {

			UE_LOG(LogSpaceshipCrew, Error, TEXT("Could not spawn mobile controls widget."));

		}

	}
}

void APlatformingPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// только add IMCs для локальный игрок контроллерs
	if (IsLocalPlayerController())
	{
 // add ввод привязки контекст
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

 // только add эти IMCs если мы not using мобильных touch ввод
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

void APlatformingPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// подписаться для pawn's OnDestroyed delegate
	InPawn->OnDestroyed.AddDynamic(this, &APlatformingPlayerController::OnPawnDestroyed);
}

void APlatformingPlayerController::OnPawnDestroyed(AActor* DestroyedActor)
{
	// find игрок start
	TArray<AActor*> ActorList;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), ActorList);

	if (ActorList.Num() > 0)
	{
 // спавн a персонаж в игрок start
		const FTransform SpawnTransform = ActorList[0]->GetActorTransform();

		if (APlatformingCharacter* RespawnedCharacter = GetWorld()->SpawnActor<APlatformingCharacter>(CharacterClass, SpawnTransform))
		{
 // possess персонаж
			Possess(RespawnedCharacter);
		}
	}
}

bool APlatformingPlayerController::ShouldUseTouchControls() const
{
	// are we на a мобильных платформа? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}




