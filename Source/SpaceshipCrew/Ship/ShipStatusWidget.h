// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ShipStatusWidget.generated.h"

class APlayerController;

/**
 * HUD-виджет отладки состояния корабля для MVP.
 * Показывает ключевые показатели систем и текущий слот/роль игрока.
 */
UCLASS(BlueprintType)
class SPACESHIPCREW_API UShipStatusWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Ship|HUD")
	void RefreshFromController(APlayerController* PlayerController);

	UFUNCTION(BlueprintPure, Category = "Ship|HUD|Format")
	float GetReactorPercent() const;

	UFUNCTION(BlueprintPure, Category = "Ship|HUD|Format")
	float GetOxygenPercent() const;

	UFUNCTION(BlueprintPure, Category = "Ship|HUD|Format")
	float GetHullPercent() const;

	UFUNCTION(BlueprintPure, Category = "Ship|HUD|Format")
	float GetFirePercent() const;

	UFUNCTION(BlueprintPure, Category = "Ship|HUD|Format")
	FText GetReactorText() const;

	UFUNCTION(BlueprintPure, Category = "Ship|HUD|Format")
	FText GetOxygenText() const;

	UFUNCTION(BlueprintPure, Category = "Ship|HUD|Format")
	FText GetHullText() const;

	UFUNCTION(BlueprintPure, Category = "Ship|HUD|Format")
	FText GetFireText() const;

	UFUNCTION(BlueprintPure, Category = "Ship|HUD|Format")
	FText GetRoleText() const;

	UFUNCTION(BlueprintPure, Category = "Ship|HUD|Format")
	FText GetSlotText() const;

	UPROPERTY(BlueprintReadOnly, Category = "Ship|HUD")
	float ReactorOutput = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Ship|HUD")
	float OxygenLevel = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Ship|HUD")
	float HullIntegrity = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Ship|HUD")
	float FireIntensity = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Ship|HUD")
	int32 CrewSlotIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Ship|HUD")
	FName CrewRoleId = NAME_None;

	/** Подсказка «нажми клавишу» при наведении лучом на станцию; обновляется в RefreshFromController. */
	UPROPERTY(BlueprintReadOnly, Category = "Ship|HUD")
	FText InteractionPromptText;

	UFUNCTION(BlueprintPure, Category = "Ship|HUD|Format")
	FText GetInteractionPromptText() const { return InteractionPromptText; }

	UFUNCTION(BlueprintPure, Category = "Ship|HUD|Format")
	bool HasInteractionPrompt() const { return !InteractionPromptText.IsEmpty(); }
};
