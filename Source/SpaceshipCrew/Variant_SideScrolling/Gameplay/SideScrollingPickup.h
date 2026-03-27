// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SideScrollingPickup.generated.h"

class USphereComponent;

/**
 * A simple side scrolling game подбор
 * Increments a counter на GameMode
 */
UCLASS(abstract)
class ASideScrollingPickup : public AActor
{
	GENERATED_BODY()
	
	/** Сфера подбора */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category ="Components", meta = (AllowPrivateAccess = "true"))
	USphereComponent* Sphere;

public:

	/** Конструктор */
	ASideScrollingPickup();

protected:

	/** Обрабатывает коллизию подбора */
	UFUNCTION()
	void BeginOverlap(AActor* OverlappedActor, AActor* OtherActor);

	/** Передаёт управление в BP для воспроизведения эффектов подбора */
	UFUNCTION(BlueprintImplementableEvent, Category="Pickup", meta = (DisplayName = "On Picked Up"))
	void BP_OnPickedUp();
};




