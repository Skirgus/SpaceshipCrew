// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CombatLifeBar.generated.h"

/**
 * A basic life bar user виджет.
 */
UCLASS(abstract)
class UCombatLifeBar : public UUserWidget
{
	GENERATED_BODY()

public:

	/** Sets life bar для provided 0-1 percentage value*/
	UFUNCTION(BlueprintImplementableEvent, Category="Life Bar")
	void SetLifePercentage(float Percent);

	// Sets life bar заполнить color
	UFUNCTION(BlueprintImplementableEvent, Category="Life Bar")
	void SetBarColor(FLinearColor Color);
};




