// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "SideScrollingAIController.generated.h"

class UStateTreeAIComponent;

/**
 * A basic AI Контроллер capable из running StateTree
 */
UCLASS(abstract)
class ASideScrollingAIController : public AAIController
{
	GENERATED_BODY()
	
	/** Компонент StateTree */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
	UStateTreeAIComponent* StateTreeAI;

public:

	/** Конструктор */
	ASideScrollingAIController();
};




