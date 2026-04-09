#include "SpaceshipCrewPlayerController.h"

#include "Engine/GameViewportClient.h"
#include "GameFramework/Pawn.h"
#include "SSpaceshipMainMenu.h"

void ASpaceshipCrewPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalPlayerController())
	{
		return;
	}

	SetIgnoreLookInput(true);
	SetIgnoreMoveInput(true);

	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;

	if (APawn* ControlledPawn = GetPawn())
	{
		ControlledPawn->DisableInput(this);
	}
	UnPossess();

	if (UWorld* World = GetWorld())
	{
		if (UGameViewportClient* ViewportClient = World->GetGameViewport())
		{
			MainMenuWidget = SNew(SSpaceshipMainMenu)
				.WorldContext(World)
				.OwnerPC(this);

			ViewportClient->AddViewportWidgetContent(MainMenuWidget.ToSharedRef(), 1000);
		}
	}

	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
}

void ASpaceshipCrewPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (MainMenuWidget.IsValid())
	{
		if (UWorld* World = GetWorld())
		{
			if (UGameViewportClient* ViewportClient = World->GetGameViewport())
			{
				ViewportClient->RemoveViewportWidgetContent(MainMenuWidget.ToSharedRef());
			}
		}
		MainMenuWidget.Reset();
	}

	Super::EndPlay(EndPlayReason);
}
