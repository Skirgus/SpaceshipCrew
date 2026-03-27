// Copyright Epic Games, Inc. All Rights Reserved.


#include "CombatAIController.h"
#include "Components/StateTreeAIComponent.h"

ACombatAIController::ACombatAIController()
{
	// создать StateTree AI Component
	StateTreeAI = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTreeAI"));
	check(StateTreeAI);

	// убедиться we начать StateTree когда we possess pawn
	bStartAILogicOnPossess = true;

	// убедиться мы attached для possessed персонаж.
	// этот is necessary для EnvQueries для work correctly
	bAttachToPawn = true;
}




