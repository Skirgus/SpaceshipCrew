// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SideScrollingInteractable.h"
#include "SideScrollingMovingPlatform.generated.h"

/**
 * Simple движущаяся платформа который can be вызванное through interactions через other акторы.
 * actual движение is performed через Blueprint code through latent execution nodes.
 */
UCLASS(abstract)
class ASideScrollingMovingPlatform : public AActor, public ISideScrollingInteractable
{
	GENERATED_BODY()
	
public:	
	
	/** Конструктор */
	ASideScrollingMovingPlatform();

protected:

	/** Если true, платформа в процессе движения и игнорирует дальнейшие взаимодействия */
	bool bMoving = false;

	/** Точка назначения платформы в мировых координатах */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Moving Platform")
	FVector PlatformTarget;

	/** Время перемещения платформы до точки назначения */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Moving Platform", meta = (ClampMin = 0, ClampMax = 10, Units="s"))
	float MoveDuration = 5.0f;

	/** Если true, платформа переместится только один раз. */
	UPROPERTY(EditAnywhere, Category="Moving Platform")
	bool bOneShot = false;

public:

// ~begin IInteractable интерфейс

	/** Выполняет взаимодействие, вызванное другим актором */
	virtual void Interaction(AActor* Interactor) override;

// ~end IInteractable интерфейс

	/** Сбрасывает состояние взаимодействия. Должно вызываться из BP-кода для сброса платформы */
	UFUNCTION(BlueprintCallable, Category="Moving Platform")
	virtual void ResetInteraction();

protected:

	/** Позволяет Blueprint-коду выполнять фактическое движение платформы */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category="Moving Platform", meta = (DisplayName="Move to Target"))
	void BP_MoveToTarget();

};




