// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SideScrollingJumpPad.generated.h"

class UBoxComponent;

/**
 * A simple прыжок pad который launches персонажs into air
 */
UCLASS(abstract)
class ASideScrollingJumpPad : public AActor
{
	GENERATED_BODY()
	
	/** Границы jump-pad (box) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UBoxComponent* Box;

protected:

	/** Vertical velocity для установить персонаж для когда they использовать прыжок pad */
	UPROPERTY(EditAnywhere, Category="Jump Pad", meta = (ClampMin=0, ClampMax=10000, Units="cm/s"))
	float ZStrength = 1000.0f;

public:	

	/** Конструктор */
	ASideScrollingJumpPad();

protected:

	UFUNCTION()
	void BeginOverlap(AActor* OverlappedActor, AActor* OtherActor);

};




