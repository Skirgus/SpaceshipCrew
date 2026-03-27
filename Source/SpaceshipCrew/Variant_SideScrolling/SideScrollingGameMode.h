// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SideScrollingGameMode.generated.h"

class USideScrollingUI;

/**
 * Simple Side Scrolling Game Mode
 * Spawns и manages game UI
 * Counts подборs collected через игрок
 */
UCLASS(abstract)
class ASideScrollingGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
protected:

	/** Класс UI-виджета, создаваемого при старте игры */
	UPROPERTY(EditAnywhere, Category="UI")
	TSubclassOf<USideScrollingUI> UserInterfaceClass;

	/** Пользовательский виджет интерфейса игры */
	UPROPERTY(BlueprintReadOnly, Category="UI")
	TObjectPtr<USideScrollingUI> UserInterface;

	/** Количество подборов, собранных игроком */
	UPROPERTY(BlueprintReadOnly, Category="Pickups")
	int32 PickupsCollected = 0;

protected:

	/** Инициализация */
	virtual void BeginPlay() override;

public:

	/** Получает событие взаимодействия от другого актора */
	virtual void ProcessPickup();
};




