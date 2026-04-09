#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SpaceshipCrew.h"
#include "SpaceshipCrewMenuGameMode.generated.h"

/**
 * GameMode стартового экрана: без пешки и без наблюдателя, только PlayerController с меню.
 */
UCLASS()
class SPACESHIPCREW_API ASpaceshipCrewMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ASpaceshipCrewMenuGameMode();
};
