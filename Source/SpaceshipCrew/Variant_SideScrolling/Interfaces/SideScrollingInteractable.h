// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SideScrollingInteractable.generated.h"

/**
 *
 */
UINTERFACE(MinimalAPI, NotBlueprintable)
class USideScrollingInteractable : public UInterface
{
	GENERATED_BODY()
};

/**
 * Simple интерфейс для allow акторы для interact without having knowledge из their internal implementation.
 */
class ISideScrollingInteractable
{
	GENERATED_BODY()

public:

	/** Triggers interaction через provided актор */
	UFUNCTION(BlueprintCallable, Category="Interactable")
	virtual void Interaction(AActor* Interactor) = 0;

};




