// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "SpaceshipCrewPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class UUserWidget;

/**
 *  Базовый класс PlayerController для игры от третьего лица
 *  Управляет контекстами ввода
 */
UCLASS(abstract)
class ASpaceshipCrewPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	/** Первый ключ из IMC проекта для действия (HUD-подсказка). */
	UFUNCTION(BlueprintPure, Category = "Input")
	FKey GetFirstKeyMappedToAction(const UInputAction* Action) const;

protected:

	/** Контексты привязки ввода */
	UPROPERTY(EditAnywhere, Category ="Input|Input Mappings")
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

	/** Инициализация игрового процесса */
	virtual void BeginPlay() override;

	/** Настройка контекстов ввода */
	virtual void SetupInputComponent() override;

	/** Возвращает true, если игрок должен использовать сенсорное управление UMG */
	bool ShouldUseTouchControls() const;

};

