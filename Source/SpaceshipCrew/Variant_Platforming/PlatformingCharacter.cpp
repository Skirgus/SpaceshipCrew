// Copyright Epic Games, Inc. All Rights Reserved.


#include "PlatformingCharacter.h"

#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "TimerManager.h"
#include "Engine/LocalPlayer.h"

APlatformingCharacter::APlatformingCharacter()
{
 	PrimaryActorTick.bCanEverTick = true;

	// инициализировать flags
	bHasWallJumped = false;
	bHasDoubleJumped = false;
	bHasDashed = false;
	bIsDashing = false;

	// привязать Делегат завершения Dash montage
	OnDashMontageEnded.BindUObject(this, &APlatformingCharacter::DashMontageEnded);

	// включить press и hold прыжок
	JumpMaxHoldTime = 0.4f;

	// установить прыжок максимальный count для 3 so we can double прыжок и проверить для coyote time прыжокs
	JumpMaxCount = 3;

	// Set size для коллизия capsule
	GetCapsuleComponent()->InitCapsuleSize(35.0f, 90.0f);

	// don't rotate mesh когда контроллер rotates
	bUseControllerRotationYaw = false;
	
	// Configure персонаж movement
	GetCharacterMovement()->GravityScale = 2.5f;
	GetCharacterMovement()->MaxAcceleration = 1500.0f;
	GetCharacterMovement()->BrakingFrictionFactor = 1.0f;
	GetCharacterMovement()->bUseSeparateBrakingFriction = true;

	GetCharacterMovement()->GroundFriction = 4.0f;
	GetCharacterMovement()->MaxWalkSpeed = 750.0f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.0f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2500.0f;
	GetCharacterMovement()->PerchRadiusThreshold = 15.0f;

	GetCharacterMovement()->JumpZVelocity = 350.0f;
	GetCharacterMovement()->BrakingDecelerationFalling = 750.0f;
	GetCharacterMovement()->AirControl = 1.0f;

	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
	GetCharacterMovement()->bOrientRotationToMovement = true;

	GetCharacterMovement()->NavAgentProps.AgentRadius = 42.0;
	GetCharacterMovement()->NavAgentProps.AgentHeight = 192.0;

	// создать камера boom
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);

	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 8.0f;
	CameraBoom->bEnableCameraRotationLag = true;	
	CameraBoom->CameraRotationLagSpeed = 8.0f;

	// создать orbiting камера
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
}

void APlatformingCharacter::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();

	// route ввод
	DoMove(MovementVector.X, MovementVector.Y);
}

void APlatformingCharacter::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// route ввод
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}


void APlatformingCharacter::Dash()
{
	// route ввод
	DoDash();
}

void APlatformingCharacter::MultiJump()
{
	// игнорировать прыжокs пока dashing
	if(bIsDashing)
		return;

	// are we already в air?
	if (GetCharacterMovement()->IsFalling())
	{

 // имеет we already wall прыжокed?
		if (!bHasWallJumped)
		{
 // run a sphere трассировка для проверить если мы в front из a wall
			FHitResult OutHit;

			const FVector TraceStart = GetActorLocation();
			const FVector TraceEnd = TraceStart + (GetActorForwardVector() * WallJumpTraceDistance);
			const FCollisionShape TraceShape = FCollisionShape::MakeSphere(WallJumpTraceRadius);

			FCollisionQueryParams QueryParams;
			QueryParams.AddIgnoredActor(this);

			if (GetWorld()->SweepSingleByChannel(OutHit, TraceStart, TraceEnd, FQuat(), ECollisionChannel::ECC_Visibility, TraceShape, QueryParams))
			{
 // rotate персонаж для face away из wall, so мы correctly oriented для следующий wall прыжок
				FRotator WallOrientation = OutHit.ImpactNormal.ToOrientationRotator();
				WallOrientation.Pitch = 0.0f;
				WallOrientation.Roll = 0.0f;

				SetActorRotation(WallOrientation);

 // apply a launch impulse для персонаж для perform actual wall прыжок
				const FVector WallJumpImpulse = (OutHit.ImpactNormal * WallJumpBounceImpulse) + (FVector::UpVector * WallJumpVerticalImpulse);

				LaunchCharacter(WallJumpImpulse, true, true);

 // включить прыжок trail
				SetJumpTrailState(true);

 // поднять wall прыжок flag для prevent immediate second wall прыжок
				bHasWallJumped = true;

				GetWorld()->GetTimerManager().SetTimer(WallJumpTimer, this, &APlatformingCharacter::ResetWallJump, DelayBetweenWallJumps, false);
			}
 // no wall прыжок, try a double прыжок next
			else
			{
 // are we still within coyote time frames?
				if (GetWorld()->GetTimeSeconds() - LastFallTime < MaxCoyoteTime)
				{
					UE_LOG(LogTemp, Warning, TEXT("Coyote Jump"));

 // использовать built-в CMC functionality для do прыжок
					Jump();

 // включить прыжок trail
					SetJumpTrailState(true);

 // no coyote time прыжок
				} else {

 // только double прыжок once пока мы в air
					if (!bHasDoubleJumped)
					{
						bHasDoubleJumped = true;

 // использовать built-в CMC functionality для do double прыжок
						Jump();

 // включить прыжок trail
						SetJumpTrailState(true);
					}

				}

				
			}
		}

	}
	else
	{
 // мы grounded so just do a regular прыжок
		Jump();

 // activate прыжок trail
		SetJumpTrailState(true);
	}
}

void APlatformingCharacter::ResetWallJump()
{
	// сбросить wall прыжок ввод lock
	bHasWallJumped = false;
}

void APlatformingCharacter::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr)
	{
 // momentarily отключить движение inputs если we've just wall прыжокed
		if (!bHasWallJumped)
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
}

void APlatformingCharacter::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{
 // add yaw и pitch ввод для контроллер
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void APlatformingCharacter::DoDash()
{
	// игнорировать ввод если we've already dashed и имеет yet для reset
	if (bHasDashed)
		return;

	// поднять dash flags
	bIsDashing = true;
	bHasDashed = true;

	// отключить gravity пока dashing
	GetCharacterMovement()->GravityScale = 0.0f;

	// сбросить персонаж velocity so we don't carry momentum into dash
	GetCharacterMovement()->Velocity = FVector::ZeroVector;

	// включить прыжок trails
	SetJumpTrailState(true);

	// play dash montage
	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		const float MontageLength = AnimInstance->Montage_Play(DashMontage, 1.0f, EMontagePlayReturnType::MontageLength, 0.0f, true);

 // has montage played successfully?
		if (MontageLength > 0.0f)
		{
			AnimInstance->Montage_SetEndDelegate(OnDashMontageEnded, DashMontage);
		}
	}
}

void APlatformingCharacter::DoJumpStart()
{
	// handle special прыжок cases
	MultiJump();
}

void APlatformingCharacter::DoJumpEnd()
{
	// stop прыжокing
	StopJumping();
}

void APlatformingCharacter::DashMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	// end dash
	EndDash();
}

void APlatformingCharacter::EndDash()
{
	// restore gravity
	GetCharacterMovement()->GravityScale = 2.5f;

	// сбросить dashing flag
	bIsDashing = false;

	// are we grounded после dash?
	if (GetCharacterMovement()->IsMovingOnGround())
	{
 // сбросить dash usage flag, since we won't receive a landed событие
		bHasDashed = false;

 // deactivate прыжок trails
		SetJumpTrailState(false);
	}
}

bool APlatformingCharacter::HasDoubleJumped() const
{
	return bHasDoubleJumped;
}

bool APlatformingCharacter::HasWallJumped() const
{
	return bHasWallJumped;
}

void APlatformingCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// очистить wall прыжок сбросить таймер
	GetWorld()->GetTimerManager().ClearTimer(WallJumpTimer);
}

void APlatformingCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action привязки
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{

 // Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &APlatformingCharacter::DoJumpStart);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &APlatformingCharacter::DoJumpEnd);

 // Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APlatformingCharacter::Move);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &APlatformingCharacter::Look);

 // Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &APlatformingCharacter::Look);

 // Dashing
		EnhancedInputComponent->BindAction(DashAction, ETriggerEvent::Triggered, this, &APlatformingCharacter::Dash);
	}
}

void APlatformingCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	// сбросить double прыжок и dash flags
	bHasDoubleJumped = false;
	bHasDashed = false;

	// deactivate прыжок trail
	SetJumpTrailState(false);
}

void APlatformingCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode /*= 0*/)
{
	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);

	// are we falling?
	if (GetCharacterMovement()->MovementMode == EMovementMode::MOVE_Falling)
	{
 // сохранить игровое время когда we started falling, so we can проверить it later для coyote time прыжокs
		LastFallTime = GetWorld()->GetTimeSeconds();
	}
}





