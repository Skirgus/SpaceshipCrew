#include "SpaceshipCrewTrainingPlayerController.h"

void ASpaceshipCrewTrainingPlayerController::BeginPlay()
{
	Super::BeginPlay();
	bShowMouseCursor = false;
	SetInputMode(FInputModeGameOnly());
}
