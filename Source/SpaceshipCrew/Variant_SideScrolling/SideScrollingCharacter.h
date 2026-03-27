// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SideScrollingCharacter.generated.h"

class UCameraComponent;
class UInputAction;
struct FInputActionValue;

/**
 * Управляемый игроком персонаж для сайд-скроллера
 */
UCLASS(abstract)
class ASideScrollingCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Камера игрока */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category ="Camera", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* Camera;

protected:

	/** Действие ввода перемещения */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MoveAction;

	/** Действие ввода прыжка */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* JumpAction;

	/** Действие схода с платформы */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* DropAction;

	/** Действие ввода взаимодействия */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* InteractAction;

	/** Импульс для ручного толчка физических объектов в воздухе */
	UPROPERTY(EditAnywhere, Category="Side Scrolling|Jump")
	float JumpPushImpulse = 600.0f;

	/** Максимальная дистанция срабатывания интерактивных объектов */
	UPROPERTY(EditAnywhere, Category="Side Scrolling|Interaction")
	float InteractionRadius = 200.0f;

	/** Время блокировки ввода после Wall Jump для сохранения инерции */
	UPROPERTY(EditAnywhere, Category="Side Scrolling|Wall Jump")
	float DelayBetweenWallJumps = 0.3f;

	/** Дистанция трассировки перед персонажем для Wall Jump */
	UPROPERTY(EditAnywhere, Category="Side Scrolling|Wall Jump")
	float WallJumpTraceDistance = 50.0f;

	/** Горизонтальный импульс, применяемый к персонажу при Wall Jump */
	UPROPERTY(EditAnywhere, Category="Side Scrolling|Wall Jump")
	float WallJumpHorizontalImpulse = 500.0f;

	/** Множитель Z-скорости прыжка для Wall Jump. */
	UPROPERTY(EditAnywhere, Category="Side Scrolling|Wall Jump")
	float WallJumpVerticalMultiplier = 1.4f;

	/** Тип объекта коллизии для трассировки мягкой коллизии (проваливание через полы) */
	UPROPERTY(EditAnywhere, Category="Side Scrolling|Soft Platforms")
	TEnumAsByte<ECollisionChannel> SoftCollisionObjectType;

	/** Дистанция трассировки вниз при проверке мягкой коллизии */
	UPROPERTY(EditAnywhere, Category="Side Scrolling|Soft Platforms")
	float SoftCollisionTraceDistance = 1000.0f;

	/** Время последнего начала падения персонажа */
	float LastFallTime = 0.0f;

	/** Максимальное время после начала падения, когда разрешён обычный прыжок */
	UPROPERTY(EditAnywhere, Category="Side Scrolling|Coyote Time", meta = (ClampMin = 0, ClampMax = 5, Units = "s"))
	float MaxCoyoteTime = 0.16f;

	/** Таймер блокировки Wall Jump */
	FTimerHandle WallJumpTimer;

	/** Последнее считанное значение горизонтального ввода движения */
	float ActionValueY = 0.0f;

	/** Последнее считанное значение оси схода с платформы */
	float DropValue = 0.0f;

	/** Если true, персонаж уже выполнял Wall Jump */
	bool bHasWallJumped = false;

	/** Если true, персонаж уже выполнял Double Jump */
	bool bHasDoubleJumped = false;

	/** Если true, персонаж движется вдоль оси сайд-скролла */
	bool bMovingHorizontally = false;

public:
	
	/** Конструктор */
	ASideScrollingCharacter();

protected:

	/** Очистка игрового состояния */
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	/** Инициализирует привязки ввод Action */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/** Обработка коллизий */
	virtual void NotifyHit(class UPrimitiveComponent* MyComp, AActor* Other, class UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit) override;

	/** Обработка приземления */
	virtual void Landed(const FHitResult& Hit) override;

	/** Обрабатывает смену режима движения для учёта прыжка с coyote time */
	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode = 0) override;

protected:

	/** Вызывается для ввода перемещения */
	void Move(const FInputActionValue& Value);

	/** Вызывается для ввода схода с платформы */
	void Drop(const FInputActionValue& Value);

	/** Вызывается для ввода схода с платформы отпускании */
	void DropReleased(const FInputActionValue& Value);

public:

	/** Обрабатывает ввод перемещения как от управления, так и от UI */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Forward);

	/** Обрабатывает ввод схода как от управления, так и от UI */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoDrop(float Value);

	/** Обрабатывает нажатие прыжка как от управления, так и от UI */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Обрабатывает нажатие прыжка как от управления, так и от UI */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();

	/** Обрабатывает ввод взаимодействия как от управления, так и от UI */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoInteract();

protected:

	/** Обрабатывает продвинутую логику прыжка */
	void MultiJump();

	/** Проверяет мягкую коллизию с платформами */
	void CheckForSoftCollision();

	/** Сбрасывает блокировку Wall Jump. Вызывается таймером после Wall Jump */
	void ResetWallJump();

public:

	/** Задаёт реакцию мягкой коллизии. True — пропускает, False — блокирует */
	void SetSoftCollision(bool bEnabled);

public:

	/** Возвращает true, если персонаж только что сделал Double Jump */
	UFUNCTION(BlueprintPure, Category="Side Scrolling")
	bool HasDoubleJumped() const;

	/** Возвращает true, если персонаж только что сделал Wall Jump */
	UFUNCTION(BlueprintPure, Category="Side Scrolling")
	bool HasWallJumped() const;
};





