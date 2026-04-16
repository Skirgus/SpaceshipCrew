#include "SpaceshipShipBuilderGameMode.h"

#include "SpaceshipShipBuilderPlayerController.h"
#include "SpaceshipShipBuilderSpectatorPawn.h"

ASpaceshipShipBuilderGameMode::ASpaceshipShipBuilderGameMode()
{
	PlayerControllerClass = ASpaceshipShipBuilderPlayerController::StaticClass();
	DefaultPawnClass = ASpaceshipShipBuilderSpectatorPawn::StaticClass();
	SpectatorClass = ASpaceshipShipBuilderSpectatorPawn::StaticClass();
	bStartPlayersAsSpectators = true;
}
