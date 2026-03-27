// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "CombatAIController.generated.h"

class UStateTreeAIComponent;

/**
 *	A basic AI Контроллер capable из running StateTree
 */
UCLASS(abstract)
class ACombatAIController : public AAIController
{
	GENERATED_BODY()

	/** Компонент StateTree */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UStateTreeAIComponent* StateTreeAI;

public:

	/** Конструктор */
	ACombatAIController();
};




