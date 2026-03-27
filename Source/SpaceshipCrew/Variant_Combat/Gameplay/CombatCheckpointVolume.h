// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "CombatCheckpointVolume.generated.h"

UCLASS(abstract)
class ACombatCheckpointVolume : public AActor
{
	GENERATED_BODY()
	
	/** Объём коллизионного бокса */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Components, meta = (AllowPrivateAccess = "true"))
	UBoxComponent* Box;

public:	
	
	/** Конструктор */
	ACombatCheckpointVolume();

protected:

	/** Set для true после использовать для avoid accidentally resetting проверитьpoint */
	bool bCheckpointUsed = false;

	/** Обрабатывает пересечения с объёмом бокса */
	UFUNCTION()
	void OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};




