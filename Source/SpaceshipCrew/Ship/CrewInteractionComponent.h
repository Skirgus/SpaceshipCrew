// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CrewInteractionComponent.generated.h"

class AController;
class AShipInteractableBase;

/** Те же шаги, что перед ServerTryInteract: луч, IA, дистанция, права. HUD и ввод используют одно правило. */
UENUM(BlueprintType)
enum class ECrewInteractAvailability : uint8
{
	None,
	NeedBindInteractAction,
	TooFar,
	NoPermission,
	/** Нет UCrewRoleComponent или RoleDefinition не задана (как на сервере в InternalApplyAction). */
	NoCrewRole,
	Ready,
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class SPACESHIPCREW_API UCrewInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCrewInteractionComponent();

	UPROPERTY(EditAnywhere, Category = "Interaction")
	float TraceDistance = 250.f;

	/** Радиус сферы вдоль луча (камера → курсор). Тонкий линейный луч часто «пролетает мимо» низких панелей; 40–60 см — компромисс. */
	UPROPERTY(EditAnywhere, Category = "Interaction", meta = (ClampMin = "0"))
	float TraceSweepRadius = 48.f;

	/** Вызывается из ввода локального игрока. */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void RequestInteract();

	/** Тот же луч, что при Interact — для HUD / подсказки «смотришь на станцию». */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	AActor* GetFocusedInteractable() const;

	/** Одна трассировка; результат согласован с RequestInteract (подсказка «Press …» только при Ready). */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	ECrewInteractAvailability GetInteractAvailability() const;

protected:
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerTryInteract(AActor* TargetActor);
	bool ServerTryInteract_Validate(AActor* TargetActor);

	AActor* TraceInteractable() const;
	bool ValidateInteractDistance(AActor* Target) const;

	ECrewInteractAvailability EvaluateInteractAvailability(AShipInteractableBase* Station) const;
};

