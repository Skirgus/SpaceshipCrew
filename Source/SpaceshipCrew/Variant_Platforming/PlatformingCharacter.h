// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Animation/AnimInstance.h"
#include "PlatformingCharacter.generated.h"


class USpringArmComponent;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;
class UAnimMontage;

/**
 * Расширенный персонаж от третьего лица со следующими возможностями:
 * - Физика движения персонажа в платформере
 * - Прыжок с удержанием
 * - Double Jump
 * - Wall Jump
 * - Dash
 */
UCLASS(abstract)
class APlatformingCharacter : public ACharacter
{
	GENERATED_BODY()

	/** CameraBoom, размещающий камеру позади персонажа */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** FollowCamera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;
	
protected:

	/** Действие ввода прыжка */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* JumpAction;

	/** Действие ввода перемещения */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MoveAction;

	/** Действие ввода обзора */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* LookAction;

	/** Действие ввода обзора мышью */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MouseLookAction;

	/** Действие ввода рывка */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* DashAction;

public:

	/** Конструктор */
	APlatformingCharacter();

protected:

	/** Вызывается для ввода перемещения */
	void Move(const FInputActionValue& Value);

	/** Вызывается для ввода обзора */
	void Look(const FInputActionValue& Value);

	/** Вызывается для ввода рывка */
	void Dash();

	/** Вызывается при нажатии прыжка для проверки условий продвинутого multi jump */
	void MultiJump();

	/** Сбрасывает блокировку ввода Wall Jump */
	void ResetWallJump();

public:

	/** Обрабатывает ввод перемещения как от управления, так и от UI */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Обрабатывает ввод обзора как от управления, так и от UI */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoLook(float Yaw, float Pitch);

	/** Обрабатывает ввод рывка как от управления, так и от UI */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoDash();

	/** Обрабатывает нажатие прыжка как от управления, так и от UI */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Обрабатывает нажатие прыжка как от управления, так и от UI */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();

protected:

	/** Вызывается делегатом при завершении dash montage */
	void DashMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	/** Передаёт управление в Blueprint для включения/выключения следов прыжка */
	UFUNCTION(BlueprintImplementableEvent, Category="Platforming")
	void SetJumpTrailState(bool bEnabled);

public:

	/** Завершает состояние рывка */
	void EndDash();

public:

	/** Возвращает true, если персонаж только что сделал Double Jump */
	UFUNCTION(BlueprintPure, Category="Platforming")
	bool HasDoubleJumped() const;

	/** Возвращает true, если персонаж только что сделал Wall Jump */
	UFUNCTION(BlueprintPure, Category="Platforming")
	bool HasWallJumped() const;

public:	
	
	/** Очистка в EndPlay */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Настраивает привязки ввод Action */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/** Обрабатывает приземления для сброса рывка и продвинутых состояний прыжка */
	virtual void Landed(const FHitResult& Hit) override;

	/** Обрабатывает смену режима движения для учёта прыжка с coyote time */
	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode = 0) override;

protected:

	/** Битовые флаги состояния движения, упакованные в uint8 для экономии памяти */
	uint8 bHasWallJumped : 1;
	uint8 bHasDoubleJumped : 1;
	uint8 bHasDashed : 1;
	uint8 bIsDashing : 1;

	/** Таймер сброса ввода Wall Jump */
	FTimerHandle WallJumpTimer;

	/** Делегат завершения Dash montage */
	FOnMontageEnded OnDashMontageEnded;

	/** Дистанция трассировки перед персонажем для поиска стены под Wall Jump */
	UPROPERTY(EditAnywhere, Category="Wall Jump", meta = (ClampMin = 0, ClampMax = 1000, Units = "cm"))
	float WallJumpTraceDistance = 50.0f;

	/** Радиус sphere trace для проверки Wall Jump */
	UPROPERTY(EditAnywhere, Category="Wall Jump", meta = (ClampMin = 0, ClampMax = 100, Units = "cm"))
	float WallJumpTraceRadius = 25.0f;

	/** Импульс отталкивания от стены при Wall Jump */
	UPROPERTY(EditAnywhere, Category="Wall Jump", meta = (ClampMin = 0, ClampMax = 10000, Units = "cm/s"))
	float WallJumpBounceImpulse = 800.0f;

	/** Вертикальный импульс при Wall Jump */
	UPROPERTY(EditAnywhere, Category="Wall Jump", meta = (ClampMin = 0, ClampMax = 10000, Units = "cm/s"))
	float WallJumpVerticalImpulse = 900.0f;

	/** Время игнорирования ввода прыжка после Wall Jump */
	UPROPERTY(EditAnywhere, Category="Wall Jump", meta = (ClampMin = 0, ClampMax = 5, Units = "s"))
	float DelayBetweenWallJumps = 0.1f;

	/** AnimMontage для использовать для Dash action */
	UPROPERTY(EditAnywhere, Category="Dash")
	UAnimMontage* DashMontage;

	/** Время последнего начала падения персонажа */
	float LastFallTime = 0.0f;

	/** Максимальное время после начала падения, когда разрешён обычный прыжок */
	UPROPERTY(EditAnywhere, Category="Coyote Time", meta = (ClampMin = 0, ClampMax = 5, Units = "s"))
	float MaxCoyoteTime = 0.16f;

public:
	/** Возвращает подобъект CameraBoom **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Возвращает подобъект FollowCamera **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

};





