// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SideScrollingInteractable.h"
#include "SideScrollingNPC.generated.h"

/**
 * Simple платформаing NPC
 * Its behaviors will be dictated через a possessing AI Контроллер
 * It can be temporarily deactivated through актор interactions
 */
UCLASS(abstract)
class ASideScrollingNPC : public ACharacter, public ISideScrollingInteractable
{
	GENERATED_BODY()

protected:

	/** Horizontal impulse для apply для NPC когда it's interacted с */
	UPROPERTY(EditAnywhere, Category="NPC", meta = (ClampMin = 0, ClampMax = 10000, Units="cm/s"))
	float LaunchImpulse = 500.0f;

	/** Vertical impulse для apply для NPC когда it's interacted с */
	UPROPERTY(EditAnywhere, Category="NPC", meta = (ClampMin = 0, ClampMax = 10000, Units="cm/s"))
	float LaunchVerticalImpulse = 500.0f;

	/** Time который NPC remains deactivated после being interacted с */
	UPROPERTY(EditAnywhere, Category="NPC", meta = (ClampMin = 0, ClampMax = 10, Units="s"))
	float DeactivationTime = 3.0f;

public:

	/** если true, этот NPC is deactivated и will not be interacted с */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="NPC")
	bool bDeactivated = false;

	/** таймер для reactivate NPC */
	FTimerHandle DeactivationTimer;

public:

	/** Конструктор */
	ASideScrollingNPC();

public:

	/** Очистка */
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

public:

//	~begin IInteractable интерфейс

	/** Выполняет взаимодействие, вызванное другим актором */
	virtual void Interaction(AActor* Interactor) override;

//	~end IInteractable интерфейс

	/** Reactivates NPC */
	void ResetDeactivation();
};




