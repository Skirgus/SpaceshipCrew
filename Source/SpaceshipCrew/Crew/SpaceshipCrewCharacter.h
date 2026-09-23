#pragma once

#include "CoreMinimal.h"
#include "CrewInteractable.h"
#include "GameFramework/Character.h"
#include "SpaceshipCrewCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;

/**
 * Пеший персонаж экипажа (3-е лицо): движение и Interact по трассировке.
 * База для BP_CrewCharacter и тренировок / кампании.
 */
UCLASS(Blueprintable)
class SPACESHIPCREW_API ASpaceshipCrewCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ASpaceshipCrewCharacter();

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Дальность трассировки Interact (см). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crew|Interact")
	float InteractTraceDistance = 250.0f;

	/** Радиус сферы overlap вокруг hit (см). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crew|Interact")
	float InteractTraceRadius = 40.0f;

	/** Текущая цель Interact (для HUD / отладки). */
	UFUNCTION(BlueprintPure, Category = "Crew|Interact")
	AActor* GetFocusedInteractable() const { return FocusedInteractable.Get(); }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Crew|Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Crew|Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	void MoveForward(float Value);
	void MoveRight(float Value);
	void TurnAtRate(float Rate);
	void LookUpAtRate(float Rate);
	void OnInteractPressed();
	void OnStationPrimaryAction();

	void UpdateFocusedInteractable();
	void PollGameplayInput(float DeltaSeconds);

	/** Станция, которую сейчас занимает этот персонаж (если есть). */
	UPROPERTY(Transient)
	TWeakObjectPtr<class ACrewWorkstation> OccupiedWorkstation;

	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> FocusedInteractable;

	UPROPERTY(EditAnywhere, Category = "Crew|Camera")
	float BaseTurnRate = 45.0f;

	UPROPERTY(EditAnywhere, Category = "Crew|Camera")
	float BaseLookUpRate = 45.0f;
};
