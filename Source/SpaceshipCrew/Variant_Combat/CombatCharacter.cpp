// Copyright Epic Games, Inc. All Rights Reserved.


#include "CombatCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "CombatLifeBar.h"
#include "Engine/DamageEvents.h"
#include "TimerManager.h"
#include "Engine/LocalPlayer.h"
#include "CombatPlayerController.h"

ACombatCharacter::ACombatCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// привязать атака montage ended delegate
	OnAttackMontageEnded.BindUObject(this, &ACombatCharacter::AttackMontageEnded);

	// Set size для коллизия capsule
	GetCapsuleComponent()->InitCapsuleSize(35.0f, 90.0f);

	// Configure персонаж movement
	GetCharacterMovement()->MaxWalkSpeed = 400.0f;

	// создать камера boom
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);

	CameraBoom->TargetArmLength = DefaultCameraDistance;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->bEnableCameraRotationLag = true;

	// создать orbiting камера
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// создать life bar виджет component
	LifeBar = CreateDefaultSubobject<UWidgetComponent>(TEXT("LifeBar"));
	LifeBar->SetupAttachment(RootComponent);

	// установить игрок tag
	Tags.Add(FName("Player"));
}

void ACombatCharacter::Move(const FInputActionValue& Value)
{
	// ввод is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	// route ввод
	DoMove(MovementVector.X, MovementVector.Y);
}

void ACombatCharacter::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// route ввод
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void ACombatCharacter::ComboAttackPressed()
{
	// route ввод
	DoComboAttackStart();
}

void ACombatCharacter::ChargedAttackPressed()
{
	// route ввод
	DoChargedAttackStart();
}

void ACombatCharacter::ChargedAttackReleased()
{
	// route ввод
	DoChargedAttackEnd();
}

void ACombatCharacter::ToggleCamera()
{
	// вызвать BP hook
	BP_ToggleCamera();
}

void ACombatCharacter::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr)
	{
 // find out which way is forward
		const FRotator Rotation = GetController()->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

 // получить вперёд vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

 // получить right vector
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

 // add movement
		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}

void ACombatCharacter::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{
 // add yaw и pitch ввод для контроллер
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void ACombatCharacter::DoComboAttackStart()
{
	// are we already playing атака animation?
	if (bIsAttacking)
	{
 // cache ввод time so we can проверить it later
		CachedAttackInputTime = GetWorld()->GetTimeSeconds();

		return;
	}

	// perform a combo атака
	ComboAttack();
}

void ACombatCharacter::DoComboAttackEnd()
{
	// заглушка
}

void ACombatCharacter::DoChargedAttackStart()
{
	// поднять charging атака flag
	bIsChargingAttack = true;

	if (bIsAttacking)
	{
 // cache ввод time so we can проверить it later
		CachedAttackInputTime = GetWorld()->GetTimeSeconds();

		return;
	}

	ChargedAttack();
}

void ACombatCharacter::DoChargedAttackEnd()
{
	// lower charging атака flag
	bIsChargingAttack = false;

	// если we've done charge цикл в least once, отпускании charged атака right away
	if (bHasLoopedChargedAttack)
	{
		CheckChargedAttack();
	}
}

void ACombatCharacter::ResetHP()
{
	// сбросить текущий HP total
	CurrentHP = MaxHP;

	// обновить life bar
	LifeBarWidget->SetLifePercentage(1.0f);
}

void ACombatCharacter::ComboAttack()
{
	// поднять атаки flag
	bIsAttacking = true;

	// сбросить combo count
	ComboCount = 0;

	// уведомить враги they are about для be атакован
	NotifyEnemiesOfIncomingAttack();

	// play атака montage
	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		const float MontageLength = AnimInstance->Montage_Play(ComboAttackMontage, 1.0f, EMontagePlayReturnType::MontageLength, 0.0f, true);

 // подписаться для montage завершённые и прерванные события
		if (MontageLength > 0.0f)
		{
 // установить делегат завершения для montage
			AnimInstance->Montage_SetEndDelegate(OnAttackMontageEnded, ComboAttackMontage);
		}
	}

}

void ACombatCharacter::ChargedAttack()
{
	// поднять атаки flag
	bIsAttacking = true;

	// сбросить charge цикл flag
	bHasLoopedChargedAttack = false;

	// уведомить враги they are about для be атакован
	NotifyEnemiesOfIncomingAttack();

	// play charged атака montage
	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		const float MontageLength = AnimInstance->Montage_Play(ChargedAttackMontage, 1.0f, EMontagePlayReturnType::MontageLength, 0.0f, true);

 // подписаться для montage завершённые и прерванные события
		if (MontageLength > 0.0f)
		{
 // установить делегат завершения для montage
			AnimInstance->Montage_SetEndDelegate(OnAttackMontageEnded, ChargedAttackMontage);
		}
	}
}

void ACombatCharacter::AttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	// сбросить атаки flag
	bIsAttacking = false;

	// проверить если we имеет a non-stale cached ввод
	if (GetWorld()->GetTimeSeconds() - CachedAttackInputTime <= AttackInputCacheTimeTolerance)
	{
 // are we holding charged атака button?
		if (bIsChargingAttack)
		{
 // do a charged атака
			ChargedAttack();
		}
		else
		{
 // do a regular атака
			ComboAttack();
		}
	}
}

void ACombatCharacter::DoAttackTrace(FName DamageSourceBone)
{
	// трассировка для объекты в front из персонаж для be попадание через атака
	TArray<FHitResult> OutHits;

	// начать в provided socket location, трассировка forward
	const FVector TraceStart = GetMesh()->GetSocketLocation(DamageSourceBone);
	const FVector TraceEnd = TraceStart + (GetActorForwardVector() * MeleeTraceDistance);

	// проверить для pawn и world dynamic коллизия объект types
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);

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
 // проверить если we've попадание a уронable актор
			ICombatDamageable* Damageable = Cast<ICombatDamageable>(CurrentHit.GetActor());

			if (Damageable)
			{
 // knock upwards и away из impact normal
				const FVector Impulse = (CurrentHit.ImpactNormal * -MeleeKnockbackImpulse) + (FVector::UpVector * MeleeLaunchImpulse);

 // передать урон событие для актор
				Damageable->ApplyDamage(MeleeDamage, this, CurrentHit.ImpactPoint, Impulse);

 // вызвать BP handler для play effects, etc.
				DealtDamage(MeleeDamage, CurrentHit.ImpactPoint);
			}
		}
	}
}

void ACombatCharacter::CheckCombo()
{
	// are we playing a non-charge атака animation?
	if (bIsAttacking && !bIsChargingAttack)
	{
 // is последний атака ввод not stale?
		if (GetWorld()->GetTimeSeconds() - CachedAttackInputTime <= ComboInputCacheTimeTolerance)
		{
 // consume атака ввод so we don't accidentally триггер it twice
			CachedAttackInputTime = 0.0f;

 // увеличить combo counter
			++ComboCount;

 // do we still имеет a combo секция для play?
			if (ComboCount < ComboSectionNames.Num())
			{
 // уведомить враги they are about для be атакован
				NotifyEnemiesOfIncomingAttack();

 // прыжок для следующий combo section
				if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
				{
					AnimInstance->Montage_JumpToSection(ComboSectionNames[ComboCount], ComboAttackMontage);
				}
			}
		}
	}
}

void ACombatCharacter::CheckChargedAttack()
{
	// поднять looped charged атака flag
	bHasLoopedChargedAttack = true;

	// прыжок для либо цикл или атака секция в зависимости на ли мы still holding charge button
	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->Montage_JumpToSection(bIsChargingAttack ? ChargeLoopSection : ChargeAttackSection, ChargedAttackMontage);
	}
}

void ACombatCharacter::NotifyEnemiesOfIncomingAttack()
{
	// трассировка для объекты в front из персонаж для be попадание через атака
	TArray<FHitResult> OutHits;

	// начать в актор location, трассировка forward
	const FVector TraceStart = GetActorLocation();
	const FVector TraceEnd = TraceStart + (GetActorForwardVector() * DangerTraceDistance);

	// проверить для pawn объект types only
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);

	// использовать a sphere форма для sweep
	FCollisionShape CollisionShape;
	CollisionShape.SetSphere(DangerTraceRadius);

	// игнорировать self
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	if (GetWorld()->SweepMultiByObjectType(OutHits, TraceStart, TraceEnd, FQuat::Identity, ObjectParams, CollisionShape, QueryParams))
	{
 // перебрать over each объект hit
		for (const FHitResult& CurrentHit : OutHits)
		{
 // проверить если we've попадание a уронable актор
			ICombatDamageable* Damageable = Cast<ICombatDamageable>(CurrentHit.GetActor());

			if (Damageable)
			{
 // уведомить враг
				Damageable->NotifyDanger(GetActorLocation(), this);
			}
		}
	}
}

void ACombatCharacter::ApplyDamage(float Damage, AActor* DamageCauser, const FVector& DamageLocation, const FVector& DamageImpulse)
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

 // передать control для BP для play effects, etc.
		ReceivedDamage(ActualDamage, DamageLocation, DamageImpulse.GetSafeNormal());
	}

}

void ACombatCharacter::HandleDeath()
{
	// отключить движение пока мы dead
	GetCharacterMovement()->DisableMovement();

	// включить full ragdoll физика
	GetMesh()->SetSimulatePhysics(true);

	// скрыть life bar
	LifeBar->SetHiddenInGame(true);

	// pull back камера
	GetCameraBoom()->TargetArmLength = DeathCameraDistance;

	// schedule респавнing
	GetWorld()->GetTimerManager().SetTimer(RespawnTimer, this, &ACombatCharacter::RespawnCharacter, RespawnTime, false);
}

void ACombatCharacter::ApplyHealing(float Healing, AActor* Healer)
{
	// заглушка
}

void ACombatCharacter::NotifyDanger(const FVector& DangerLocation, AActor* DangerSource)
{
	// заглушка
}

void ACombatCharacter::RespawnCharacter()
{
	// уничтожить персонаж и let it be респавнed через Игрок Контроллер
	Destroy();
}

float ACombatCharacter::TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
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

void ACombatCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	// is персонаж still alive?
	if (CurrentHP >= 0.0f)
	{
 // отключить ragdoll физика
		GetMesh()->SetPhysicsBlendWeight(0.0f);
	}
}

void ACombatCharacter::BeginPlay()
{
	Super::BeginPlay();

	// получить life bar из виджет component
	LifeBarWidget = Cast<UCombatLifeBar>(LifeBar->GetUserWidgetObject());
	check(LifeBarWidget);

	// инициализировать камера
	GetCameraBoom()->TargetArmLength = DefaultCameraDistance;

	// сохранить relative transform для mesh so we can сбросить ragdoll later
	MeshStartingTransform = GetMesh()->GetRelativeTransform();

	// установить life bar color
	LifeBarWidget->SetBarColor(LifeBarColor);

	// сбросить HP для maximum
	ResetHP();
}

void ACombatCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// очистить респавн таймер
	GetWorld()->GetTimerManager().ClearTimer(RespawnTimer);
}

void ACombatCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Set up action привязки
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
 // Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ACombatCharacter::Move);

 // Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ACombatCharacter::Look);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &ACombatCharacter::Look);

 // Combo Атака
		EnhancedInputComponent->BindAction(ComboAttackAction, ETriggerEvent::Started, this, &ACombatCharacter::ComboAttackPressed);

 // Charged Атака
		EnhancedInputComponent->BindAction(ChargedAttackAction, ETriggerEvent::Started, this, &ACombatCharacter::ChargedAttackPressed);
		EnhancedInputComponent->BindAction(ChargedAttackAction, ETriggerEvent::Completed, this, &ACombatCharacter::ChargedAttackReleased);

 // Камера Side Toggle
		EnhancedInputComponent->BindAction(ToggleCameraAction, ETriggerEvent::Triggered, this, &ACombatCharacter::ToggleCamera);
	}
}

void ACombatCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	// обновить респавн transform на Игрок Контроллер
	if (ACombatPlayerController* PC = Cast<ACombatPlayerController>(GetController()))
	{
		PC->SetRespawnTransform(GetActorTransform());
	}
}





