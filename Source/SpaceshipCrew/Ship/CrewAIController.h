// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "CrewAIController.generated.h"

/**
 * AI-контроллер для ботов экипажа; использует ту же пешку и тот же путь взаимодействия, что и игрок.
 */
UCLASS()
class SPACESHIPCREW_API ACrewAIController : public AAIController
{
	GENERATED_BODY()

public:
	ACrewAIController();
};

