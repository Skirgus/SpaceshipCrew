#include "SpaceshipShipBuilderGameMode.h"

#include "GameFramework/SpectatorPawn.h"
#include "SpaceshipShipBuilderPlayerController.h"

ASpaceshipShipBuilderGameMode::ASpaceshipShipBuilderGameMode()
{
	PlayerControllerClass = ASpaceshipShipBuilderPlayerController::StaticClass();
	DefaultPawnClass = ASpectatorPawn::StaticClass();
	SpectatorClass = ASpectatorPawn::StaticClass();
	bStartPlayersAsSpectators = true;
}
