#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SpaceshipCrew.h"
#include "SpaceshipShipBuilderGameMode.generated.h"

/**
 * Режим конструктора корабля: отдельный PlayerController с UI (T02c-1).
 */
UCLASS()
class SPACESHIPCREW_API ASpaceshipShipBuilderGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ASpaceshipShipBuilderGameMode();
};
