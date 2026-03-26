// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShipInteractableBase.generated.h"

class UShipSystemsComponent;
class AController;

/**
 * World interactable that forwards authorized actions to UShipSystemsComponent.
 * Bots and players use the same ExecuteInteract path.
 */
UCLASS(Blueprintable)
class SPACESHIPCREW_API AShipInteractableBase : public AActor
{
	GENERATED_BODY()

public:
	AShipInteractableBase();

	/** Actor with UShipSystemsComponent (e.g. AShipActor). If null, first AShipActor in world is used. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship")
	TObjectPtr<AActor> OwningShipActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship")
	FName RequiredPermission;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship")
	FName ActionId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship")
	float Magnitude = 1.f;

	/** Server-only: apply interaction (call from Server RPC on interaction component). */
	UFUNCTION(BlueprintCallable, Category = "Ship")
	void ExecuteInteract(AController* Issuer);

	UFUNCTION(BlueprintPure, Category = "Ship")
	UShipSystemsComponent* ResolveShipSystems() const;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Ship")
	TObjectPtr<class USceneComponent> RootScene;
};
