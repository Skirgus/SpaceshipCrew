// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SpaceshipCrewCharacter.h"
#include "ShipCrewCharacter.generated.h"

class UInputAction;
struct FInputActionValue;
class AController;
class UCrewRoleComponent;
class UCrewInteractionComponent;
class UCrewBotBrainComponent;

/**
 * Конкретная пешка экипажа от третьего лица с ролью и взаимодействием (для игрока и ботов).
 */
UCLASS()
class SPACESHIPCREW_API AShipCrewCharacter : public ASpaceshipCrewCharacter
{
	GENERATED_BODY()

public:
	AShipCrewCharacter();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Crew")
	TObjectPtr<UCrewRoleComponent> CrewRole;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Crew")
	TObjectPtr<UCrewInteractionComponent> CrewInteraction;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Crew")
	TObjectPtr<UCrewBotBrainComponent> CrewBotBrain;

	/** Необязательно: привяжите в BP или назначьте IA_Interact. */
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> InteractAction;

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;

	/** Использует переданный IC (из Setup) или InputComponent после Possess. */
	void TryBindInteractInput(UInputComponent* IC);

	void BindInteractFallbackNextTick();

	void OnInteractPressed(const FInputActionValue& Value);

	bool bInteractInputBound = false;
};

