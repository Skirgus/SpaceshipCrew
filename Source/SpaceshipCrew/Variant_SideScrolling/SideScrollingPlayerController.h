// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "EnhancedInput/Public/InputAction.h"
#include "SideScrollingPlayerController.generated.h"

class ASideScrollingCharacter;
class UInputMappingContext;

/**
 * A simple Side Scrolling Игрок Контроллер
 * Управляет контекстами ввода
 * Reспавнs игрок pawn в игрок начать если it is destroyed
 */
UCLASS(abstract, Config="Game")
class ASideScrollingPlayerController : public APlayerController
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
	TSubclassOf<ASideScrollingCharacter> CharacterClass;

protected:

	/** Инициализация игрового процесса */
	virtual void BeginPlay() override;

	/** Инициализация привязок ввода */
	virtual void SetupInputComponent() override;

	/** Инициализация пешки */
	virtual void OnPossess(APawn* InPawn) override;

	/** Вызывается при уничтожении управляемой пешки */
	UFUNCTION()
	void OnPawnDestroyed(AActor* DestroyedActor);

	/** Возвращает true, если игрок должен использовать сенсорное управление UMG */
	bool ShouldUseTouchControls() const;

};




