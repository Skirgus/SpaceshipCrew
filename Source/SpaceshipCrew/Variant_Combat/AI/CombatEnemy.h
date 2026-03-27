// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "CombatAttacker.h"
#include "CombatDamageable.h"
#include "Animation/AnimMontage.h"
#include "Engine/TimerHandle.h"
#include "CombatEnemy.generated.h"

class UWidgetComponent;
class UCombatLifeBar;
class UAnimMontage;

/** Completed атака animation delegate для StateTree */
DECLARE_DELEGATE(FOnEnemyAttackCompleted);

/** Landed delegate для StateTree */
DECLARE_DELEGATE(FOnEnemyLanded);

/** Враг died delegate */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEnemyDied);

/**
 * An AI-controlled персонаж с combat capabilities.
 * Its bundled AI Контроллер runs logic through StateTree
 */
UCLASS(abstract)
class ACombatEnemy : public ACharacter, public ICombatAttacker, public ICombatDamageable
{
	GENERATED_BODY()

	/** Life bar виджет component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UWidgetComponent* LifeBar;

public:
	
	/** Конструктор */
	ACombatEnemy();

protected:

	/** Max количество из HP персонаж will имеет на респавн */
	UPROPERTY(EditAnywhere, Category="Damage")
	float MaxHP = 3.0f;

public:

	/** Current количество из HP персонаж has */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Damage", meta = (ClampMin = 0, ClampMax = 100))
	float CurrentHP = 0.0f;

protected:

	/** Name из pelvis bone, для урон ragdoll физика */
	UPROPERTY(EditAnywhere, Category="Damage")
	FName PelvisBoneName;

	/** Pointer для life bar виджет */
	UPROPERTY(EditAnywhere, Category="Damage")
	UCombatLifeBar* LifeBarWidget;

	/** если true, персонаж is currently playing атака animation */
	bool bIsAttacking = false;

	/** Distance ahead из персонаж который melee атака sphere коллизия traces will extend */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Trace", meta = (ClampMin = 0, ClampMax = 500, Units = "cm"))
	float MeleeTraceDistance = 75.0f;

	/** Radius из sphere trace для melee атакаs */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Trace", meta = (ClampMin = 0, ClampMax = 500, Units = "cm"))
	float MeleeTraceRadius = 50.0f;

	/** Amount из урон a melee атака will deal */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Damage", meta = (ClampMin = 0, ClampMax = 100))
	float MeleeDamage = 1.0f;

	/** Amount из knockback impulse a melee атака will apply */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Damage", meta = (ClampMin = 0, ClampMax = 1000, Units = "cm/s"))
	float MeleeKnockbackImpulse = 150.0f;

	/** Amount из upwards impulse a melee атака will apply */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Damage", meta = (ClampMin = 0, ClampMax = 1000, Units = "cm/s"))
	float MeleeLaunchImpulse = 350.0f;

	/** AnimMontage который will play для combo атакаs */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Combo")
	UAnimMontage* ComboAttackMontage;

	/** Names из AnimMontage секции который correspond для each stage из combo атака */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Combo")
	TArray<FName> ComboSectionNames;

	/** цель number из атакаs в combo атака серии мы playing */
	int32 TargetComboCount = 0;

	/** Index из текущий stage из melee атака combo */
	int32 CurrentComboAttack = 0;

	/** AnimMontage который will play для charged атакаs */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Charged")
	UAnimMontage* ChargedAttackMontage;

	/** Name из AnimMontage секция который corresponds для charge цикл */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Charged")
	FName ChargeLoopSection;

	/** Name из AnimMontage секция который corresponds для атака */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Charged")
	FName ChargeAttackSection;

	/** Minimum number из charge animation циклов который will be played через AI */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Charged", meta = (ClampMin = 1, ClampMax = 20))
	int32 MinChargeLoops = 2;

	/** Maximum number из charge animation циклов который will be played через AI */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Charged", meta = (ClampMin = 1, ClampMax = 20))
	int32 MaxChargeLoops = 5;

	/** цель number из charge animation циклов для play в этот charged атака */
	int32 TargetChargeLoops = 0;

	/** Number из charge animation цикл currently playing */
	int32 CurrentChargeLoop = 0;

	/** Time для wait до reдвижущаяся этот персонаж из level после it dies */
	UPROPERTY(EditAnywhere, Category="Death")
	float DeathRemovalTime = 5.0f;

	/** Враг death таймер */
	FTimerHandle DeathTimer;

	/** Атака montage ended delegate */
	FOnMontageEnded OnAttackMontageEnded;

	/** Last recorded позиция мы being атакован из */
	FVector LastDangerLocation = FVector::ZeroVector;

	/** Last recorded игровое время we were атакован */
	float LastDangerTime = -1000.0f;

public:
	/** Атака завершённые internal delegate для уведомить StateTree tasks */
	FOnEnemyAttackCompleted OnAttackCompleted;

	/** Landed internal delegate для уведомить StateTree tasks. We использовать этот instead из built-в Landed delegate so we can привязать для a Lambda в StateTree tasks */
	FOnEnemyLanded OnEnemyLanded;

	/** Враг died delegate. позволяет external подписчиков для respond для враг death */
	UPROPERTY(BlueprintAssignable, Category="Events")
	FOnEnemyDied OnEnemyDied;

public:

	/** Выполняет AI-initiated combo атака. Number из hits will be decided через этот персонаж */
	void DoAIComboAttack();

	/** Выполняет AI-initiated charged атака. Charge time will be decided через этот персонаж */
	void DoAIChargedAttack();

	/** Вызывается из a delegate когда атака montage ends */
	void AttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	/** Возвращает последний recorded позиция we were атакован из */
	const FVector& GetLastDangerLocation() const;

	/** Возвращает последний игровое время we were атакован */
	float GetLastDangerTime() const;

public:

	// ~begin ICombatАтакаer интерфейс

	/** Выполняет атака's коллизия проверить */
	virtual void DoAttackTrace(FName DamageSourceBone) override;

	/** Выполняет a combo атака's проверить для continue серии */
	UFUNCTION(BlueprintCallable, Category="Attacker")
	virtual void CheckCombo() override;

	/** Выполняет a charged атака's проверить для цикл charge animation */
	UFUNCTION(BlueprintCallable, Category="Attacker")
	virtual void CheckChargedAttack() override;

	// ~end ICombatАтакаer интерфейс

	// ~begin ICombatDamageable интерфейс

	/** Обрабатывает урон и отбрасывание */
	virtual void ApplyDamage(float Damage, AActor* DamageCauser, const FVector& DamageLocation, const FVector& DamageImpulse) override;

	/** Обрабатывает события смерти */
	virtual void HandleDeath() override;

	/** Обрабатывает события лечения */
	virtual void ApplyHealing(float Healing, AActor* Healer) override;

	/** позволяет враг для react для входящий атакаs */
	virtual void NotifyDanger(const FVector& DangerLocation, AActor* DangerSource) override;

	// ~end ICombatDamageable интерфейс

protected:

	/** Removes этот персонаж из level после it dies */
	void RemoveFromLevel();

public:

	/** Overrides по умолчанию TakeDamage functionality */
	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	/** Overrides landing для сбросить урон ragdoll физика */
	virtual void Landed(const FHitResult& Hit) override;

protected:

	/** Blueprint handler для play урон полученный effects */
	UFUNCTION(BlueprintImplementableEvent, Category="Combat")
	void ReceivedDamage(float Damage, const FVector& ImpactPoint, const FVector& DamageDirection);

protected:

	/** Инициализация игрового процесса */
	virtual void BeginPlay() override;

	/** Очистка в EndPlay */
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;
};




