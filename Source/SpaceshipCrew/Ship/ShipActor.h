// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShipActor.generated.h"

class UShipSystemsComponent;

/** Placed in level; holds authoritative ship systems. */
UCLASS()
class SPACESHIPCREW_API AShipActor : public AActor
{
	GENERATED_BODY()

public:
	AShipActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ship")
	TObjectPtr<UShipSystemsComponent> ShipSystems;

	/** Optional spawn offsets for crew (local to ship). If empty, uses ship origin + row spacing. */
	UPROPERTY(EditAnywhere, Category = "Ship|Crew")
	TArray<FTransform> CrewSpawnTransforms;

	UFUNCTION(BlueprintPure, Category = "Ship")
	UShipSystemsComponent* GetShipSystems() const { return ShipSystems; }
};
