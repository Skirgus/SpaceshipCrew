#include "SpaceshipCrewMenuGameMode.h"

#include "SpaceshipCrewPlayerController.h"

ASpaceshipCrewMenuGameMode::ASpaceshipCrewMenuGameMode()
{
	PlayerControllerClass = ASpaceshipCrewPlayerController::StaticClass();
	DefaultPawnClass = nullptr;
	SpectatorClass = nullptr;
	bStartPlayersAsSpectators = false;
}
