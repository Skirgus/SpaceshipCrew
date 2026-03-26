// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "CrewAIController.generated.h"

/**
 * AI controller for bot crew; uses same pawn and interaction path as players.
 */
UCLASS()
class SPACESHIPCREW_API ACrewAIController : public AAIController
{
	GENERATED_BODY()

public:
	ACrewAIController();
};
