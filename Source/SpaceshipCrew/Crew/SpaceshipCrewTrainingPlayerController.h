#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SpaceshipCrewTrainingPlayerController.generated.h"

/**
 * PC тренировки: игровой ввод (не UI-only как меню).
 */
UCLASS()
class SPACESHIPCREW_API ASpaceshipCrewTrainingPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
};
