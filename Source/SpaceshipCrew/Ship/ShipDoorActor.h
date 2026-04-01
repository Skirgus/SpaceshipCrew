// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShipDoorActor.generated.h"

class UStaticMeshComponent;

UCLASS()
class SPACESHIPCREW_API AShipDoorActor : public AActor
{
	GENERATED_BODY()

public:
	AShipDoorActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Door")
	TObjectPtr<UStaticMeshComponent> DoorMesh;

	UFUNCTION(BlueprintCallable, Category = "Door")
	void SetDoorOpen(bool bOpen);
};

