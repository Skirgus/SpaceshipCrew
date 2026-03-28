// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SpaceshipCrewPlayerController.h"
#include "ShipPlayerController.generated.h"

class UShipStatusWidget;

/** Конкретный PlayerController для режима корабля (базовый класс шаблона абстрактный). */
UCLASS()
class SPACESHIPCREW_API AShipPlayerController : public ASpaceshipCrewPlayerController
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnPossess(APawn* InPawn) override;

protected:
	/** Игровой ввод + скрытый курсор: чтобы мышь шла в Look, а не «застревала» в UI после HUD. */
	void ApplyShipViewAndInputDefaults();

	UFUNCTION()
	void RefreshShipHud();

	/** Класс HUD-виджета с показателями корабля (задаётся в BP). */
	UPROPERTY(EditAnywhere, Category = "Ship|HUD")
	TSubclassOf<UShipStatusWidget> ShipStatusWidgetClass;

	UPROPERTY()
	TObjectPtr<UShipStatusWidget> ShipStatusWidget;

	/** Интервал обновления показателей в HUD. */
	UPROPERTY(EditAnywhere, Category = "Ship|HUD", meta = (ClampMin = "0.05", ClampMax = "2.0"))
	float HudRefreshInterval = 0.2f;

	FTimerHandle HudRefreshTimer;
};

