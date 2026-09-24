#pragma once

#include "CoreMinimal.h"
#include "CrewInteractable.h"
#include "GameFramework/Character.h"
#include "UI/UsableEquipmentPromptWidget.h"
#include "SpaceshipCrewCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class AUsableEquipment;
class UInventoryComponent;
class UCrewHotbarWidget;
class UCrewInventoryWidget;

/**
 * Пеший персонаж экипажа (3-е лицо): движение, оборудование, инвентарь.
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
	virtual void PossessedBy(AController* NewController) override;
	virtual void Tick(float DeltaSeconds) override;

	/** Виджет подсказки «E». Пустой класс — стандартный C++ виджет. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crew|Interact")
	TSubclassOf<UUsableEquipmentPromptWidget> PromptWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crew|Inventory")
	TSubclassOf<UCrewHotbarWidget> HotbarWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crew|Inventory")
	TSubclassOf<UCrewInventoryWidget> InventoryWidgetClass;

	/** Текущая цель Interact (для HUD / отладки). */
	UFUNCTION(BlueprintPure, Category = "Crew|Interact")
	AActor* GetFocusedInteractable() const { return FocusedInteractable.Get(); }

	UFUNCTION(BlueprintPure, Category = "Crew|Inventory")
	UInventoryComponent* GetInventory() const { return Inventory; }

	/** true, если персонаж сидит / occupy с блокировкой шага. */
	UFUNCTION(BlueprintPure, Category = "Crew|Interact")
	bool IsMovementLockedByEquipment() const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Crew|Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Crew|Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Crew|Inventory")
	TObjectPtr<UInventoryComponent> Inventory;

	void MoveForward(float Value);
	void MoveRight(float Value);
	void TurnAtRate(float Rate);
	void LookUpAtRate(float Rate);
	void OnInteractPressed();

	void UpdateFocusedInteractable();
	void PollGameplayInput(float DeltaSeconds);
	void EnsurePromptWidget();
	void EnsureInventoryWidgets();
	void RefreshInteractPrompt();
	void PollHeldItemUse(APlayerController* PC);
	void PollHotbarAndInventory(APlayerController* PC, float DeltaSeconds);

	/** Оборудование, которое сейчас использует этот персонаж. */
	UPROPERTY(Transient)
	TWeakObjectPtr<AUsableEquipment> ActiveEquipment;

	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> FocusedInteractable;

	UPROPERTY(Transient)
	TObjectPtr<UUsableEquipmentPromptWidget> PromptWidget;

	UPROPERTY(Transient)
	TObjectPtr<UCrewHotbarWidget> HotbarWidget;

	UPROPERTY(Transient)
	TObjectPtr<UCrewInventoryWidget> InventoryWidget;

	UPROPERTY(EditAnywhere, Category = "Crew|Camera")
	float BaseTurnRate = 45.0f;

	UPROPERTY(EditAnywhere, Category = "Crew|Camera")
	float BaseLookUpRate = 45.0f;

	/** Макс. наклон вниз (град.). */
	UPROPERTY(EditAnywhere, Category = "Crew|Camera", meta = (ClampMin = "10", ClampMax = "89"))
	float CameraLookDownPitchMax = 60.0f;

	/** Макс. наклон вверх. */
	UPROPERTY(EditAnywhere, Category = "Crew|Camera", meta = (ClampMin = "-89", ClampMax = "-10"))
	float CameraLookUpPitchMin = -70.0f;

	/** Базовая длина пружины. Укорачивается только при коллизии с потолком/стеной. */
	UPROPERTY(EditAnywhere, Category = "Crew|Camera")
	float CameraArmLength = 280.0f;

	UPROPERTY(EditAnywhere, Category = "Crew|Camera")
	FVector CameraSocketOffset = FVector(0.0f, 50.0f, 20.0f);

	UPROPERTY(EditAnywhere, Category = "Crew|Camera")
	FVector CameraBoomLocation = FVector(0.0f, 0.0f, 50.0f);

	/** Плавность отъезда/приближения при коллизии spring arm. */
	UPROPERTY(EditAnywhere, Category = "Crew|Camera", meta = (ClampMin = "0.1", ClampMax = "30"))
	float CameraCollisionLagSpeed = 8.0f;

	UPROPERTY(EditAnywhere, Category = "Crew|Inventory")
	float ClearActiveHoldSeconds = 0.4f;

	void ApplyCameraPitchLimits();
	void ClampControlPitch();
	static float NormalizePitchDegrees(float Pitch);

	float ClearActiveHoldTime = 0.0f;
	bool bClearActiveTriggered = false;
};
