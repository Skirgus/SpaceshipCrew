// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CombatDamageable.h"
#include "CombatDamageableBox.generated.h"

/**
 * A simple физика box который reacts для урон through ICombatDamageable интерфейс
 */
UCLASS(abstract)
class ACombatDamageableBox : public AActor, public ICombatDamageable
{
	GENERATED_BODY()
	
	/** Меш разрушаемого ящика */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* Mesh;

public:	

	/** Конструктор */
	ACombatDamageableBox();

protected:

	/** Начальное количество HP у ящика. */
	UPROPERTY(EditAnywhere, Category="Damage")
	float CurrentHP = 3.0f;

	/** Время ожидания перед удалением ящика с уровня. */
	UPROPERTY(EditAnywhere, Category="Damage", meta = (ClampMin = 0, ClampMax = 10, Units = "s"))
	float DeathDelayTime = 6.0f;

	/** Таймер отложенного удаления ящика после исчерпания HP */
	FTimerHandle DeathTimer;

	/** Blueprint-обработчик урона для воспроизведения эффектов */
	UFUNCTION(BlueprintImplementableEvent, Category="Damage")
	void OnBoxDamaged(const FVector& DamageLocation, const FVector& DamageImpulse);

	/** Blueprint-обработчик уничтожения для воспроизведения эффектов */
	UFUNCTION(BlueprintImplementableEvent, Category="Damage")
	void OnBoxDestroyed();

	/** Таймерный callback для удаления ящика с уровня после уничтожения */
	void RemoveFromLevel();

public:

	/** Очистка в EndPlay */
	void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	// ~Begin CombatDamageable интерфейс

	/** Обрабатывает урон и отбрасывание */
	virtual void ApplyDamage(float Damage, AActor* DamageCauser, const FVector& DamageLocation, const FVector& DamageImpulse) override;

	/** Обрабатывает события смерти */
	virtual void HandleDeath() override;

	/** Обрабатывает события лечения */
	virtual void ApplyHealing(float Healing, AActor* Healer) override;

	/** Позволяет реагировать на входящие атаки */
	virtual void NotifyDanger(const FVector& DangerLocation, AActor* DangerSource) override;

	// ~End CombatDamageable интерфейс
};




