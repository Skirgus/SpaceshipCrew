#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SpaceshipCrew.h"
#include "SpaceshipCrewPlayerController.generated.h"

class SSpaceshipMainMenu;

UCLASS()
class SPACESHIPCREW_API ASpaceshipCrewPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	TSharedPtr<SSpaceshipMainMenu> MainMenuWidget;
};
