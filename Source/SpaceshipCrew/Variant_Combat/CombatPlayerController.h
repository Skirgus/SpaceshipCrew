// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CombatPlayerController.generated.h"

class UInputMappingContext;
class ACombatCharacter;

/**
 * Простой PlayerController для боевой игры от третьего лица
 * Управляет контекстами ввода
 * Респавнит персонажа игрока в контрольной точке после его уничтожения
 */
UCLASS(abstract, Config="Game")
class ACombatPlayerController : public APlayerController
{
	GENERATED_BODY()
	
protected:

	/** Контекст привязки ввода для этого игрока */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/** Контексты привязки ввода */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	/** Виджет мобильного управления для создания */
	UPROPERTY(EditAnywhere, Category="Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	/** Указатель на виджет мобильного управления */
	UPROPERTY()
	TObjectPtr<UUserWidget> MobileControlsWidget;

	/** Если true, игрок использует сенсорное управление UMG даже вне мобильных платформ */
	UPROPERTY(EditAnywhere, Config, Category = "Input|Touch Controls")
	bool bForceTouchControls = false;

	/** Класс персонажа для повторного спавна после уничтожения управляемой пешки */
	UPROPERTY(EditAnywhere, Category="Respawn")
	TSubclassOf<ACombatCharacter> CharacterClass;

	/** Transform для респавна персонажа. Может быть задан для создания контрольных точек */
	FTransform RespawnTransform;

protected:

	/** Инициализация игрового процесса */
	virtual void BeginPlay() override;

	/** Инициализация привязок ввода */
	virtual void SetupInputComponent() override;

	/** Инициализация пешки */
	virtual void OnPossess(APawn* InPawn) override;

public:

	/** Обновляет Transform респавна персонажа */
	void SetRespawnTransform(const FTransform& NewRespawn);

protected:

	/** Вызывается при уничтожении управляемой пешки */
	UFUNCTION()
	void OnPawnDestroyed(AActor* DestroyedActor);

	/** Возвращает true, если игрок должен использовать сенсорное управление UMG */
	bool ShouldUseTouchControls() const;

};




