// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SideScrollingSoftPlatform.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UBoxComponent;

/**
 * A side scrolling game платформа который персонаж can прыжок или drop through.
 */
UCLASS(abstract)
class ASideScrollingSoftPlatform : public AActor
{
	GENERATED_BODY()
	
	/** Корневой компонент */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category ="Components", meta = (AllowPrivateAccess = "true"))
	USceneComponent* Root;

	/** Меш платформы. Часть, с которой сталкиваемся и которую видим */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category ="Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* Mesh;

	/** Коллизионный объём, переключающий мягкую коллизию у персонажа, когда он ниже платформы. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category ="Components", meta = (AllowPrivateAccess = "true"))
	UBoxComponent* CollisionCheckBox;

public:	
	
	/** Конструктор */
	ASideScrollingSoftPlatform();

protected:

	/** Обрабатывает пересечения в зоне проверки мягкой коллизии */
	UFUNCTION()
	void OnSoftCollisionOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** Восстанавливает состояние мягкой коллизии по окончании пересечения */
	virtual void NotifyActorEndOverlap(AActor* OtherActor) override;
};




