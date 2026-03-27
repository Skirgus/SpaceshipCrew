// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "SpaceshipCrewCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 *  Простой управляемый игроком персонаж от третьего лица
 *  Реализует управляемую орбитальную камеру
 */
UCLASS(abstract)
class ASpaceshipCrewCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Камера-бум, размещающая камеру позади персонажа */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Камера следования */
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

public:

	/** Конструктор */
	ASpaceshipCrewCharacter();	

protected:

	/** Инициализирует привязки действий ввода */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:

	/** Вызывается для ввода перемещения */
	void Move(const FInputActionValue& Value);

	/** Вызывается для ввода обзора */
	void Look(const FInputActionValue& Value);

public:

	/** Обрабатывает ввод перемещения как от управления, так и от UI */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Обрабатывает ввод обзора как от управления, так и от UI */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoLook(float Yaw, float Pitch);

	/** Обрабатывает нажатие прыжка как от управления, так и от UI */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Обрабатывает нажатие прыжка как от управления, так и от UI */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();

public:

	/** Возвращает подобъект CameraBoom **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Возвращает подобъект FollowCamera **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};


