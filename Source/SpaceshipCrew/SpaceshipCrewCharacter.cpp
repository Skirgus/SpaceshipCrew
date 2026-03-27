// Copyright Epic Games, Inc. All Rights Reserved.

#include "SpaceshipCrewCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "SpaceshipCrew.h"

ASpaceshipCrewCharacter::ASpaceshipCrewCharacter()
{
	// Настраиваем размер коллизионной капсулы
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Не вращаем персонажа при повороте контроллера — это влияет только на камеру.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Настраиваем движение персонажа
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	// Примечание: для более быстрой итерации эти и многие другие параметры можно настраивать в Character Blueprint
	// вместо перекомпиляции проекта для каждого изменения
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Создаём камеру-бум (приближает камеру к игроку при столкновении)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	// Создаём камеру следования
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Примечание: ссылки на скелетный меш и anim blueprint на компоненте Mesh (унаследованном от Character) 
	// задаются в наследуемом blueprint-ассете ThirdPersonCharacter (чтобы избежать прямых ссылок на контент в C++)
}

void ASpaceshipCrewCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Настраиваем привязки действий
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Прыжок
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Перемещение
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ASpaceshipCrewCharacter::Move);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &ASpaceshipCrewCharacter::Look);

		// Обзор
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ASpaceshipCrewCharacter::Look);
	}
	else
	{
		UE_LOG(LogSpaceshipCrew, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void ASpaceshipCrewCharacter::Move(const FInputActionValue& Value)
{
	// ввод имеет тип Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	// передаём ввод дальше
	DoMove(MovementVector.X, MovementVector.Y);
}

void ASpaceshipCrewCharacter::Look(const FInputActionValue& Value)
{
	// ввод имеет тип Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// передаём ввод дальше
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void ASpaceshipCrewCharacter::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr)
	{
		// определяем направление вперёд
		const FRotator Rotation = GetController()->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// получаем вектор вперёд
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// получаем вектор вправо 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// добавляем перемещение 
		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}

void ASpaceshipCrewCharacter::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{
		// добавляем yaw/pitch ввод в контроллер
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void ASpaceshipCrewCharacter::DoJumpStart()
{
	// даём персонажу команду на прыжок
	Jump();
}

void ASpaceshipCrewCharacter::DoJumpEnd()
{
	// даём персонажу команду прекратить прыжок
	StopJumping();
}

