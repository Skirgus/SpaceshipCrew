#pragma once

#include "CoreMinimal.h"
#include "CrewInteractable.h"
#include "GameFramework/Character.h"
#include "UI/UsableEquipmentPromptWidget.h"
#include "SpaceshipCrewCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class AUsableEquipment;

/**
 * Пеший персонаж экипажа (3-е лицо): движение и взаимодействие с оборудованием по зоне.
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

	/** Виджет подсказки «E». Пустой класс — стандартный C++ виджет. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crew|Interact")
	TSubclassOf<UUsableEquipmentPromptWidget> PromptWidgetClass;

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

	void UpdateFocusedInteractable();
	void PollGameplayInput(float DeltaSeconds);
	void EnsurePromptWidget();
	void RefreshInteractPrompt();

	/** Оборудование, которое сейчас использует этот персонаж. */
	UPROPERTY(Transient)
	TWeakObjectPtr<AUsableEquipment> ActiveEquipment;

	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> FocusedInteractable;

	UPROPERTY(Transient)
	TObjectPtr<UUsableEquipmentPromptWidget> PromptWidget;

	UPROPERTY(EditAnywhere, Category = "Crew|Camera")
	float BaseTurnRate = 45.0f;

	UPROPERTY(EditAnywhere, Category = "Crew|Camera")
	float BaseLookUpRate = 45.0f;
};
