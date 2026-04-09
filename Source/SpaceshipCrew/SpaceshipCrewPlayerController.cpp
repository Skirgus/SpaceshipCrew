#include "SpaceshipCrewPlayerController.h"

#include "Engine/GameViewportClient.h"
#include "SSpaceshipMainMenu.h"

void ASpaceshipCrewPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalPlayerController())
	{
		return;
	}

	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;

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
