// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CrewBotBrainComponent.generated.h"

class AShipInteractableBase;

/**
 * Lightweight task director for MVP bots: priority reacts to ship state and uses the same interactables as players.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class SPACESHIPCREW_API UCrewBotBrainComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCrewBotBrainComponent();

	/** GameMode enables this for AI-controlled crew only. */
	UPROPERTY(EditAnywhere, Category = "Bot")
	bool bBrainEnabled = false;

	UPROPERTY(EditAnywhere, Category = "Bot")
	float ThinkInterval = 0.75f;

	UPROPERTY(EditAnywhere, Category = "Bot")
	float AcceptableInteractDistance = 180.f;

protected:
	virtual void BeginPlay() override;
	void Think();

	FTimerHandle ThinkTimer;

	AShipInteractableBase* FindBestInteractable() const;
	void TryMoveAndInteract(AShipInteractableBase* Target);
};
