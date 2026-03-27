// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CrewInteractionComponent.generated.h"

class AController;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class SPACESHIPCREW_API UCrewInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCrewInteractionComponent();

	UPROPERTY(EditAnywhere, Category = "Interaction")
	float TraceDistance = 250.f;

	/** Вызывается из ввода локального игрока. */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void RequestInteract();

protected:
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerTryInteract(AActor* TargetActor);
	bool ServerTryInteract_Validate(AActor* TargetActor);

	AActor* TraceInteractable() const;
	bool ValidateInteractDistance(AActor* Target) const;
};

