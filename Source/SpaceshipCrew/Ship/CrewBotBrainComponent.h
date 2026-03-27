// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CrewBotBrainComponent.generated.h"

class AShipInteractableBase;

/**
 * Лёгкий директор задач для MVP-ботов: приоритеты зависят от состояния корабля и используют те же interactable, что и игрок.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class SPACESHIPCREW_API UCrewBotBrainComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCrewBotBrainComponent();

	/** GameMode включает это только для экипажа под управлением ИИ. */
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

