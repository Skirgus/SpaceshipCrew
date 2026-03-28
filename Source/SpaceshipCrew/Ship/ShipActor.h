// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShipActor.generated.h"

class UShipSystemsComponent;
class UStaticMeshComponent;

/** Размещается на уровне; содержит авторитетные системы корабля. */
UCLASS(Blueprintable)
class SPACESHIPCREW_API AShipActor : public AActor
{
	GENERATED_BODY()

public:
	AShipActor();

	/** Визуал и корень трансформации корабля; меш задаётся в BP или на инстансе. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ship|Visual")
	TObjectPtr<UStaticMeshComponent> HullMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ship")
	TObjectPtr<UShipSystemsComponent> ShipSystems;

	/** Необязательные смещения спавна экипажа (локально к кораблю). Если пусто, используется центр корабля + шаг по ряду. */
	UPROPERTY(EditAnywhere, Category = "Ship|Crew")
	TArray<FTransform> CrewSpawnTransforms;

	UFUNCTION(BlueprintPure, Category = "Ship")
	UShipSystemsComponent* GetShipSystems() const { return ShipSystems; }
};

