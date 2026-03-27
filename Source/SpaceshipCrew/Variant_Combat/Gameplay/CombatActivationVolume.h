// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CombatActivationVolume.generated.h"

class UBoxComponent;

/**
 * A simple volume который активирует a список из акторы когда игрок pawn enters.
 */
UCLASS()
class ACombatActivationVolume : public AActor
{
	GENERATED_BODY()

	/** Объём коллизионного бокса */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category ="Components", meta = (AllowPrivateAccess = "true"))
	UBoxComponent* Box;
	
protected:

	/** Список акторов, активируемых при входе в этот объём */
	UPROPERTY(EditAnywhere, Category="Activation Volume")
	TArray<AActor*> ActorsToActivate;

public:	
	
	/** Конструктор */
	ACombatActivationVolume();

protected:

	/** Обрабатывает пересечения с объёмом бокса */
	UFUNCTION()
	void OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

};




