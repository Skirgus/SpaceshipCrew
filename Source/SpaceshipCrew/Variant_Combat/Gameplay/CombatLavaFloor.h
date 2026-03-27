// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CombatLavaFloor.generated.h"

class UStaticMeshComponent;
class UPrimitiveComponent;

/**
 * A basic актор который applies урон на contact through ICombatDamageable интерфейс.
 */
UCLASS(abstract)
class ACombatLavaFloor : public AActor
{
	GENERATED_BODY()
	
	/** Floor mesh */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* Mesh;

protected:

	/** Amount из урон для deal на contact */
	UPROPERTY(EditAnywhere, Category="Damage")
	float Damage = 10000.0f;

public:	

	/** Конструктор */
	ACombatLavaFloor();

protected:

	/** Blocking попадание handler */
	UFUNCTION()
	void OnFloorHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
};




