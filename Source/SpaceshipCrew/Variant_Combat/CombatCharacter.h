// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "CombatAttacker.h"
#include "CombatDamageable.h"
#include "Animation/AnimInstance.h"
#include "CombatCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;
class UCombatLifeBar;
class UWidgetComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogCombatCharacter, Log, All);

/**
 * An enhanced Third Person Персонаж с melee combat capabilities:
 * - Combo атака string
 * - Press и hold charged атака
 * - Damage dealing и reaction
 * - Death
 * - Reспавнing
 */
UCLASS(abstract)
class ACombatCharacter : public ACharacter, public ICombatAttacker, public ICombatDamageable
{
	GENERATED_BODY()

	/** Камера boom positioning камера behind персонаж */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Камера следования */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	/** Life bar виджет component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UWidgetComponent* LifeBar;
	
protected:

	/** Действие ввода прыжка */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* JumpAction;

	/** Действие ввода перемещения */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* MoveAction;

	/** Действие ввода обзора */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* LookAction;

	/** Действие ввода обзора мышью */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MouseLookAction;

	/** Combo Атака ввод Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* ComboAttackAction;

	/** Charged Атака ввод Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* ChargedAttackAction;

	/** Toggle Камера Side ввод Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* ToggleCameraAction;

	/** Max количество из HP персонаж will имеет на респавн */
	UPROPERTY(EditAnywhere, Category="Damage", meta = (ClampMin = 0, ClampMax = 100))
	float MaxHP = 5.0f;

	/** Current количество из HP персонаж has */
	UPROPERTY(VisibleAnywhere, Category="Damage")
	float CurrentHP = 0.0f;

	/** Life bar виджет заполнить color */
	UPROPERTY(EditAnywhere, Category="Damage")
	FLinearColor LifeBarColor;

	/** Name из pelvis bone, для урон ragdoll физика */
	UPROPERTY(EditAnywhere, Category="Damage")
	FName PelvisBoneName;

	/** Pointer для life bar виджет */
	UPROPERTY(EditAnywhere, Category="Damage")
	TObjectPtr<UCombatLifeBar> LifeBarWidget;

	/** Max количество из time который may elapse для a non-combo атака ввод для not be considered stale */
	UPROPERTY(EditAnywhere, Category="Melee Attack", meta = (ClampMin = 0, ClampMax = 5, Units = "s"))
	float AttackInputCacheTimeTolerance = 1.0f;

	/** Time в which атака button was последний pressed */
	float CachedAttackInputTime = 0.0f;

	/** если true, персонаж is currently playing атака animation */
	bool bIsAttacking = false;

	/** Distance ahead из персонаж который melee атака sphere коллизия traces will extend */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Trace", meta = (ClampMin = 0, ClampMax = 500, Units="cm"))
	float MeleeTraceDistance = 75.0f;

	/** Radius из sphere trace для melee атакаs */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Trace", meta = (ClampMin = 0, ClampMax = 200, Units = "cm"))
	float MeleeTraceRadius = 75.0f;

	/** Distance ahead из персонаж который враги will be notified из входящий атакаs */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Trace", meta = (ClampMin = 0, ClampMax = 500, Units="cm"))
	float DangerTraceDistance = 300.0f;

	/** Radius из sphere trace для уведомить враги из входящий атакаs */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Trace", meta = (ClampMin = 0, ClampMax = 200, Units = "cm"))
	float DangerTraceRadius = 100.0f;

	/** Amount из урон a melee атака will deal */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Damage", meta = (ClampMin = 0, ClampMax = 100))
	float MeleeDamage = 1.0f;

	/** Amount из knockback impulse a melee атака will apply */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Damage", meta = (ClampMin = 0, ClampMax = 1000, Units = "cm/s"))
	float MeleeKnockbackImpulse = 250.0f;

	/** Amount из upwards impulse a melee атака will apply */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Damage", meta = (ClampMin = 0, ClampMax = 1000, Units = "cm/s"))
	float MeleeLaunchImpulse = 300.0f;

	/** AnimMontage который will play для combo атакаs */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Combo")
	UAnimMontage* ComboAttackMontage;

	/** Names из AnimMontage секции который correspond для each stage из combo атака */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Combo")
	TArray<FName> ComboSectionNames;

	/** Max количество из time который may elapse для a combo атака ввод для not be considered stale */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Combo", meta = (ClampMin = 0, ClampMax = 5, Units = "s"))
	float ComboInputCacheTimeTolerance = 0.45f;

	/** Index из текущий stage из melee атака combo */
	int32 ComboCount = 0;

	/** AnimMontage который will play для charged атакаs */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Charged")
	UAnimMontage* ChargedAttackMontage;

	/** Name из AnimMontage секция который corresponds для charge цикл */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Charged")
	FName ChargeLoopSection;

	/** Name из AnimMontage секция который corresponds для атака */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Charged")
	FName ChargeAttackSection;

	/** Flag который determines если игрок is currently holding charged атака ввод */
	bool bIsChargingAttack = false;
	
	/** если true, charged атака hold проверить has been tested в least once */
	bool bHasLoopedChargedAttack = false;

	/** Камера boom length пока персонаж is мёртв */
	UPROPERTY(EditAnywhere, Category="Camera", meta = (ClampMin = 0, ClampMax = 1000, Units = "cm"))
	float DeathCameraDistance = 400.0f;

	/** Камера boom length когда персонаж респавнs */
	UPROPERTY(EditAnywhere, Category="Camera", meta = (ClampMin = 0, ClampMax = 1000, Units = "cm"))
	float DefaultCameraDistance = 100.0f;

	/** Time для wait до респавнing персонаж */
	UPROPERTY(EditAnywhere, Category="Respawn", meta = (ClampMin = 0, ClampMax = 10, Units = "s"))
	float RespawnTime = 3.0f;

	/** Атака montage ended delegate */
	FOnMontageEnded OnAttackMontageEnded;

	/** Персонаж респавн таймер */
	FTimerHandle RespawnTimer;

	/** Copy из mesh's transform so we can сбросить it после ragdoll animations */
	FTransform MeshStartingTransform;

public:
	
	/** Конструктор */
	ACombatCharacter();

protected:

	/** Вызывается для ввода перемещения */
	void Move(const FInputActionValue& Value);

	/** Вызывается для ввода обзора */
	void Look(const FInputActionValue& Value);

	/** Вызывается для combo атака ввод */
	void ComboAttackPressed();

	/** Вызывается для combo атака ввод pressed */
	void ChargedAttackPressed();

	/** Вызывается для combo атака ввод отпусканииd */
	void ChargedAttackReleased();

	/** Вызывается для toggle камера side ввод */
	void ToggleCamera();

	/** BP hook для animate камера side switch */
	UFUNCTION(BlueprintImplementableEvent, Category="Combat")
	void BP_ToggleCamera();

public:

	/** Обрабатывает ввод перемещения как от управления, так и от UI */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Обрабатывает ввод обзора как от управления, так и от UI */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoLook(float Yaw, float Pitch);

	/** Обрабатывает combo атака pressed из либо controls или UI интерфейсов */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoComboAttackStart();

	/** Обрабатывает combo атака отпусканииd из либо controls или UI интерфейсов */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoComboAttackEnd();

	/** Обрабатывает charged атака pressed из либо controls или UI интерфейсов */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoChargedAttackStart();

	/** Обрабатывает charged атака отпусканииd из либо controls или UI интерфейсов */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoChargedAttackEnd();

protected:

	/** Resets персонаж's текущий HP для maximum */
	void ResetHP();

	/** Выполняет a combo атака */
	void ComboAttack();

	/** Выполняет a charged атака */
	void ChargedAttack();

	/** Вызывается из a delegate когда атака montage ends */
	void AttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	
public:

	// ~begin CombatАтакаer интерфейс

	/** Выполняет коллизия проверить для атака */
	virtual void DoAttackTrace(FName DamageSourceBone) override;

	/** Выполняет combo серии проверить */
	virtual void CheckCombo() override;

	/** Выполняет charged атака hold проверить */
	virtual void CheckChargedAttack() override;

	// ~end CombatАтакаer интерфейс

	// ~begin CombatDamageable интерфейс

	/** Notifies nearby враги который атака is coming so they can react */
	void NotifyEnemiesOfIncomingAttack();

	/** Обрабатывает урон и отбрасывание */
	virtual void ApplyDamage(float Damage, AActor* DamageCauser, const FVector& DamageLocation, const FVector& DamageImpulse) override;

	/** Обрабатывает события смерти */
	virtual void HandleDeath() override;

	/** Обрабатывает события лечения */
	virtual void ApplyHealing(float Healing, AActor* Healer) override;

	/** Позволяет реагировать на входящие атаки */
	virtual void NotifyDanger(const FVector& DangerLocation, AActor* DangerSource) override;

	// ~end CombatDamageable интерфейс

	/** Вызывается из респавн таймер для уничтожить и re-создать персонаж */
	void RespawnCharacter();

public:

	/** Overrides по умолчанию TakeDamage functionality */
	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	/** Overrides landing для сбросить урон ragdoll физика */
	virtual void Landed(const FHitResult& Hit) override;

protected:

	/** Blueprint handler для play урон dealt effects */
	UFUNCTION(BlueprintImplementableEvent, Category="Combat")
	void DealtDamage(float Damage, const FVector& ImpactPoint);

	/** Blueprint handler для play урон полученный effects */
	UFUNCTION(BlueprintImplementableEvent, Category="Combat")
	void ReceivedDamage(float Damage, const FVector& ImpactPoint, const FVector& DamageDirection);

protected:

	/** Инициализация */
	virtual void BeginPlay() override;

	/** Очистка */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Обрабатывает ввод привязки */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/** Обрабатывает possessed Инициализация */
	virtual void NotifyControllerChanged() override;

public:

	/** Возвращает подобъект CameraBoom **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Возвращает подобъект FollowCamera **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};




