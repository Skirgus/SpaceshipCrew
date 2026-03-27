// Copyright Epic Games, Inc. All Rights Reserved.


#include "CombatEnemy.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "CombatAIController.h"
#include "Components/WidgetComponent.h"
#include "Engine/DamageEvents.h"
#include "CombatLifeBar.h"
#include "TimerManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"

ACombatEnemy::ACombatEnemy()
{
	PrimaryActorTick.bCanEverTick = true;

	// Привязываем делегат завершения Attack montage
	OnAttackMontageEnded.BindUObject(this, &ACombatEnemy::AttackMontageEnded);

	// Устанавливаем класс AIController по умолчанию
	AIControllerClass = ACombatAIController::StaticClass();

	// Используем AIController как для размещенного, так и для заспавненного врага
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// Игнорируем поворот yaw от Controller
	bUseControllerRotationYaw = false;

	// Создаем виджет полосы здоровья
	LifeBar = CreateDefaultSubobject<UWidgetComponent>(TEXT("LifeBar"));
	LifeBar->SetupAttachment(RootComponent);

	// Настраиваем размер капсулы коллизии
	GetCapsuleComponent()->SetCapsuleSize(35.0f, 90.0f);

	// Настраиваем параметры движения персонажа
	GetCharacterMovement()->bUseControllerDesiredRotation = true;

	// Сбрасываем HP до максимума
	CurrentHP = MaxHP;
}

void ACombatEnemy::DoAIComboAttack()
{
	// Игнорируем вызов, если уже проигрывается анимация атаки
	if (bIsAttacking)
	{
		return;
	}

	// Поднимаем флаг атаки
	bIsAttacking = true;

	// Выбираем количество атак в серии
	TargetComboCount = FMath::RandRange(1, ComboSectionNames.Num() - 1);

	// Сбрасываем счетчик атак
	CurrentComboAttack = 0;

	// Запускаем Attack montage
	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		const float MontageLength = AnimInstance->Montage_Play(ComboAttackMontage, 1.0f, EMontagePlayReturnType::MontageLength, 0.0f, true);

 // Подписываемся на события завершения и прерывания montage
		if (MontageLength > 0.0f)
		{
 // Устанавливаем делегат завершения montage
			AnimInstance->Montage_SetEndDelegate(OnAttackMontageEnded, ComboAttackMontage);
		}
	}
}

void ACombatEnemy::DoAIChargedAttack()
{
	// Игнорируем вызов, если уже проигрывается анимация атаки
	if (bIsAttacking)
	{
		return;
	}

	// Поднимаем флаг атаки
	bIsAttacking = true;

	// выбрать how many циклов are we going для charge для
	TargetChargeLoops = FMath::RandRange(MinChargeLoops, MaxChargeLoops);

	// сбросить charge цикл counter
	CurrentChargeLoop = 0;

	// Запускаем Attack montage
	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		const float MontageLength = AnimInstance->Montage_Play(ChargedAttackMontage, 1.0f, EMontagePlayReturnType::MontageLength, 0.0f, true);

 // Подписываемся на события завершения и прерывания montage
		if (MontageLength > 0.0f)
		{
 // Устанавливаем делегат завершения montage
			AnimInstance->Montage_SetEndDelegate(OnAttackMontageEnded, ChargedAttackMontage);
		}
	}
}

void ACombatEnemy::AttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	// сбросить атаки flag
	bIsAttacking = false;

	// вызвать атака завершённые delegate so StateTree can continue execution
	OnAttackCompleted.ExecuteIfBound();
}

const FVector& ACombatEnemy::GetLastDangerLocation() const
{
	return LastDangerLocation;
}

float ACombatEnemy::GetLastDangerTime() const
{
	return LastDangerTime;
}

void ACombatEnemy::DoAttackTrace(FName DamageSourceBone)
{
	// трассировка для объекты в front из персонаж для be попадание через атака
	TArray<FHitResult> OutHits;

	// начать в provided socket location, трассировка forward
	const FVector TraceStart = GetMesh()->GetSocketLocation(DamageSourceBone);
	const FVector TraceEnd = TraceStart + (GetActorForwardVector() * MeleeTraceDistance);

	// враги только влияют на Pawn коллизия objects; they don't knock back boxes
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);

	// использовать a sphere форма для sweep
	FCollisionShape CollisionShape;
	CollisionShape.SetSphere(MeleeTraceRadius);

	// игнорировать self
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	if (GetWorld()->SweepMultiByObjectType(OutHits, TraceStart, TraceEnd, FQuat::Identity, ObjectParams, CollisionShape, QueryParams))
	{
 // перебрать over each объект hit
		for (const FHitResult& CurrentHit : OutHits)
		{
 /** does актор имеет игрок tag? */
			if (CurrentHit.GetActor()->ActorHasTag(FName("Player")))
			{
 // проверить если актор is уронable
				ICombatDamageable* Damageable = Cast<ICombatDamageable>(CurrentHit.GetActor());

				if (Damageable)
				{
 // knock upwards и away из impact normal
					const FVector Impulse = (CurrentHit.ImpactNormal * -MeleeKnockbackImpulse) + (FVector::UpVector * MeleeLaunchImpulse);

 // передать урон событие для актор
					Damageable->ApplyDamage(MeleeDamage, this, CurrentHit.ImpactPoint, Impulse);

				}
			}
		}
	}
}

void ACombatEnemy::CheckCombo()
{
	// увеличить combo counter
	++CurrentComboAttack;

	// do we still имеет атакаs для play в этот string?
	if (CurrentComboAttack < TargetComboCount)
	{
 // прыжок для следующий атака section
		if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
		{
			AnimInstance->Montage_JumpToSection(ComboSectionNames[CurrentComboAttack], ComboAttackMontage);
		}
	}
}

void ACombatEnemy::CheckChargedAttack()
{
	// увеличить charge цикл counter
	++CurrentChargeLoop;

	// прыжок для либо цикл или атака секция из montage в зависимости на ли we попадание цикл цель
	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->Montage_JumpToSection(CurrentChargeLoop >= TargetChargeLoops ? ChargeAttackSection : ChargeLoopSection, ChargedAttackMontage);
	}
}

void ACombatEnemy::ApplyDamage(float Damage, AActor* DamageCauser, const FVector& DamageLocation, const FVector& DamageImpulse)
{
	
	// передать урон событие для актор
	FDamageEvent DamageEvent;
	const float ActualDamage = TakeDamage(Damage, DamageEvent, nullptr, DamageCauser);

	// только process knockback и effects если we полученный nonzero урон
	if (ActualDamage > 0.0f)
	{
 // apply knockback impulse
		GetCharacterMovement()->AddImpulse(DamageImpulse, true);

 // is персонаж ragdolling?
		if (GetMesh()->IsSimulatingPhysics())
		{
 // apply impulse для ragdoll
			GetMesh()->AddImpulseAtLocation(DamageImpulse * GetMesh()->GetMass(), DamageLocation);
		}

 // stop атака montages для interrupt атака
		if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
		{
			AnimInstance->Montage_Stop(0.1f, ComboAttackMontage);
			AnimInstance->Montage_Stop(0.1f, ChargedAttackMontage);
		}

 // передать control для BP для play effects, etc.
		ReceivedDamage(ActualDamage, DamageLocation, DamageImpulse.GetSafeNormal());
	}
}

void ACombatEnemy::HandleDeath()
{
	// скрыть life bar
	LifeBar->SetHiddenInGame(true);

	// отключить коллизия capsule для avoid being попадание again пока dead
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// отключить персонаж movement
	GetCharacterMovement()->DisableMovement();

	// включить full ragdoll физика
	GetMesh()->SetSimulatePhysics(true);

	// вызвать died delegate для уведомить any subscribers
	OnEnemyDied.Broadcast();

	// установить up death таймер
	GetWorld()->GetTimerManager().SetTimer(DeathTimer, this, &ACombatEnemy::RemoveFromLevel, DeathRemovalTime);
}

void ACombatEnemy::ApplyHealing(float Healing, AActor* Healer)
{
	// заглушка
}

void ACombatEnemy::NotifyDanger(const FVector& DangerLocation, AActor* DangerSource)
{
	// убедиться мы being атакован через игрок
	if (DangerSource && DangerSource->ActorHasTag(FName("Player")))
	{
 // сохранить опасность позиция и game time
		LastDangerLocation = DangerLocation;
		LastDangerTime = GetWorld()->GetTimeSeconds();
	}
}

void ACombatEnemy::RemoveFromLevel()
{
	// уничтожить этот актор
	Destroy();
}

float ACombatEnemy::TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// только process урон если персонаж is still alive
	if (CurrentHP <= 0.0f)
	{
		return 0.0f;
	}

	// уменьшить текущий HP
	CurrentHP -= Damage;

	// имеет we run out из HP?
	if (CurrentHP <= 0.0f)
	{
 // die
		HandleDeath();
	}
	else
	{
 // обновить life bar
		LifeBarWidget->SetLifePercentage(CurrentHP / MaxHP);

 // включить partial ragdoll физика, but сохранить pelvis vertical
		GetMesh()->SetPhysicsBlendWeight(0.5f);
		GetMesh()->SetBodySimulatePhysics(PelvisBoneName, false);
	}

	// вернуть полученный урон amount
	return Damage;
}

void ACombatEnemy::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	// is персонаж still alive?
	if (CurrentHP >= 0.0f)
	{
 // отключить ragdoll физика
		GetMesh()->SetPhysicsBlendWeight(0.0f);
	}

	// вызвать landed Delegate для StateTree
	OnEnemyLanded.ExecuteIfBound();
}

void ACombatEnemy::BeginPlay()
{
	// Сбрасываем HP до максимума
	CurrentHP = MaxHP;

	// we top HP до BeginPlay so StateTree picks it up в right value
	Super::BeginPlay();

	// получить life bar виджет из виджет comp
	LifeBarWidget = Cast<UCombatLifeBar>(LifeBar->GetUserWidgetObject());
	check(LifeBarWidget);

	// заполнить life bar
	LifeBarWidget->SetLifePercentage(1.0f);
}

void ACombatEnemy::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// очистить death таймер
	GetWorld()->GetTimerManager().ClearTimer(DeathTimer);
}





