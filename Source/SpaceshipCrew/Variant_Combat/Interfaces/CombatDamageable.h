// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CombatDamageable.generated.h"

/**
 * CombatDamageable интерфейс
 * предоставляет functionality для handle урон, лечение, knockback и death
 * Also предоставляет functionality для warn персонажs из входящий sources из урон
 */
UINTERFACE(MinimalAPI, NotBlueprintable)
class UCombatDamageable : public UInterface
{
	GENERATED_BODY()
};

class ICombatDamageable
{
	GENERATED_BODY()

public:

	/** Обрабатывает урон и отбрасывание */
	UFUNCTION(BlueprintCallable, Category="Damageable")
	virtual void ApplyDamage(float Damage, AActor* DamageCauser, const FVector& DamageLocation, const FVector& DamageImpulse) = 0;

	/** Обрабатывает события смерти */
	UFUNCTION(BlueprintCallable, Category="Damageable")
	virtual void HandleDeath() = 0;

	/** Обрабатывает события лечения */
	UFUNCTION(BlueprintCallable, Category="Damageable")
	virtual void ApplyHealing(float Healing, AActor* Healer) = 0;

	/** Notifies актор из impending опасность such as входящий hit, allowing it для react. */
	UFUNCTION(BlueprintCallable, Category="Damageable")
	virtual void NotifyDanger(const FVector& DangerLocation, AActor* DangerSource) = 0;
};




