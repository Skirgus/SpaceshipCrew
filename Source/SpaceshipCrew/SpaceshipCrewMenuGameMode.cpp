#include "SpaceshipCrewMenuGameMode.h"

#include "SpaceshipCrewPlayerController.h"

ASpaceshipCrewMenuGameMode::ASpaceshipCrewMenuGameMode()
{
	PlayerControllerClass = ASpaceshipCrewPlayerController::StaticClass();
	DefaultPawnClass = nullptr;
	bStartPlayersAsSpectators = true;
}
