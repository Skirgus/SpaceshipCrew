// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShipActor.generated.h"

class UShipSystemsComponent;

/** Размещается на уровне; содержит авторитетные системы корабля. */
UCLASS()
class SPACESHIPCREW_API AShipActor : public AActor
{
	GENERATED_BODY()

public:
	AShipActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ship")
	TObjectPtr<UShipSystemsComponent> ShipSystems;

	/** Необязательные смещения спавна экипажа (локально к кораблю). Если пусто, используется центр корабля + шаг по ряду. */
	UPROPERTY(EditAnywhere, Category = "Ship|Crew")
	TArray<FTransform> CrewSpawnTransforms;

	UFUNCTION(BlueprintPure, Category = "Ship")
	UShipSystemsComponent* GetShipSystems() const { return ShipSystems; }
};

