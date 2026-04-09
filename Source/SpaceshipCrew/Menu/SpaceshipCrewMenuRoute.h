#pragma once

#include "CoreMinimal.h"
#include "SpaceshipCrewMenuRoute.generated.h"

UENUM()
enum class ESpaceshipMenuRoute : uint8
{
	Constructor,
	Trainings,
	NewGame,
	Settings,
	Exit
};

namespace SpaceshipCrewMenu
{
	TArray<ESpaceshipMenuRoute> GetOrderedMenuRoutes();
	FText GetDisplayName(ESpaceshipMenuRoute Route);
	bool IsExitRoute(ESpaceshipMenuRoute Route);
}
