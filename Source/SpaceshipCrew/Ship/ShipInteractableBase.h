// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShipInteractableBase.generated.h"

class UShipSystemsComponent;
class AController;
class APawn;

/**
 * Интерактивный объект мира, который передаёт авторизованные действия в UShipSystemsComponent.
 * Боты и игроки используют один и тот же путь ExecuteInteract.
 */
UCLASS(Blueprintable)
class SPACESHIPCREW_API AShipInteractableBase : public AActor
{
	GENERATED_BODY()

public:
	AShipInteractableBase();

	/** Актор с UShipSystemsComponent (например, AShipActor). Если не задан, используется первый AShipActor в мире. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship")
	TObjectPtr<AActor> OwningShipActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship")
	FName RequiredPermission;

	/**
	 * Должен совпадать с веткой в UShipSystemsComponent::InternalApplyAction.
	 * Примеры: IncreaseReactorPower, DecreaseReactorPower, EnableOxygenSupply, DisableOxygenSupply.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship")
	FName ActionId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship")
	float Magnitude = 1.f;

	/** Только на сервере: применить взаимодействие (вызов из Server RPC компонента взаимодействия). */
	UFUNCTION(BlueprintCallable, Category = "Ship")
	void ExecuteInteract(AController* Issuer, APawn* InstigatorPawn = nullptr);

	UFUNCTION(BlueprintPure, Category = "Ship")
	UShipSystemsComponent* ResolveShipSystems() const;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Ship")
	TObjectPtr<class USceneComponent> RootScene;

	/** Коллизия для гарантированного наведения трассой взаимодействия (ECC_Visibility). */
	UPROPERTY(VisibleAnywhere, Category = "Ship")
	TObjectPtr<class USphereComponent> InteractionCollider;
};

