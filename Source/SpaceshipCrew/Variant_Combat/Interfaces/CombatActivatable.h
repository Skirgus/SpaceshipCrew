// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CombatActivatable.generated.h"

/**
 * Interactable Interface
 * предоставляет a контекст-agnostic way из activating, deactivating или toggling акторы
 */
UINTERFACE(MinimalAPI, NotBlueprintable)
class UCombatActivatable : public UInterface
{
	GENERATED_BODY()
};

class ICombatActivatable
{
	GENERATED_BODY()

public:

	/** Toggles Interactable актор */
	UFUNCTION(BlueprintCallable, Category="Activatable")
	virtual void ToggleInteraction(AActor* ActivationInstigator) = 0;

	/** Activates Interactable актор */
	UFUNCTION(BlueprintCallable, Category="Activatable")
	virtual void ActivateInteraction(AActor* ActivationInstigator) = 0;

	/** Deactivates Interactable актор */
	UFUNCTION(BlueprintCallable, Category="Activatable")
	virtual void DeactivateInteraction(AActor* ActivationInstigator) = 0;
};




