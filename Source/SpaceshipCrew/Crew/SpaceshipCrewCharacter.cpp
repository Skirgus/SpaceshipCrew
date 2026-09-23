#include "SpaceshipCrewCharacter.h"

#include "CrewWorkstation.h"
#include "EngineerEnergyConsole.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "CrewInteractable.h"
#include "Engine/Engine.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputCoreTypes.h"
#include "UObject/ConstructorHelpers.h"

ASpaceshipCrewCharacter::ASpaceshipCrewCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
	GetCharacterMovement()->MaxWalkSpeed = 450.0f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	// Точка крепления у груди/головы: при сжатии у стены камера не уходит в центр капсулы (внутрь меша).
	CameraBoom->SetRelativeLocation(FVector(0.0f, 0.0f, 55.0f));
	CameraBoom->TargetArmLength = 350.0f;
	CameraBoom->SocketOffset = FVector(0.0f, 55.0f, 40.0f);
	CameraBoom->bDoCollisionTest = true;
	CameraBoom->ProbeSize = 14.0f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Пакеты шаблона UE — /Game/Characters/Mannequins (не /Game/Mannequins).
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MannyMesh(
		TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
	static ConstructorHelpers::FClassFinder<UAnimInstance> MannyAnim(
		TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed.ABP_Unarmed_C"));

	USkeletalMeshComponent* SkelMesh = GetMesh();
	SkelMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -96.0f));
	SkelMesh->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	SkelMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (MannyMesh.Succeeded())
	{
		SkelMesh->SetSkeletalMeshAsset(MannyMesh.Object);
	}
	if (MannyAnim.Succeeded())
	{
		SkelMesh->SetAnimInstanceClass(MannyAnim.Class);
	}
}

void ASpaceshipCrewCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Fallback, если Live Coding / soft path не подхватил ассет в конструкторе.
	USkeletalMeshComponent* SkelMesh = GetMesh();
	if (SkelMesh && !SkelMesh->GetSkeletalMeshAsset())
	{
		if (USkeletalMesh* Manny = LoadObject<USkeletalMesh>(
				nullptr, TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple")))
		{
			SkelMesh->SetSkeletalMeshAsset(Manny);
			SkelMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -96.0f));
			SkelMesh->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
		}
	}
	if (SkelMesh && !SkelMesh->GetAnimClass())
	{
		if (UClass* AnimClass = LoadClass<UAnimInstance>(
				nullptr, TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed.ABP_Unarmed_C")))
		{
			SkelMesh->SetAnimInstanceClass(AnimClass);
		}
	}
}

void ASpaceshipCrewCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	(void)PlayerInputComponent;
	// Enhanced Input — дефолт проекта; legacy BindAxis на нём ненадёжен.
	// Игровой ввод читаем в Tick через APlayerController (WASD / мышь / E / 1).
}

void ASpaceshipCrewCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	PollGameplayInput(DeltaSeconds);
	UpdateFocusedInteractable();
}

void ASpaceshipCrewCharacter::PollGameplayInput(float DeltaSeconds)
{
	APlayerController* PC = Cast<APlayerController>(Controller);
	if (!PC)
	{
		return;
	}

	float Forward = 0.0f;
	float Right = 0.0f;
	if (PC->IsInputKeyDown(EKeys::W) || PC->IsInputKeyDown(EKeys::Up))
	{
		Forward += 1.0f;
	}
	if (PC->IsInputKeyDown(EKeys::S) || PC->IsInputKeyDown(EKeys::Down))
	{
		Forward -= 1.0f;
	}
	if (PC->IsInputKeyDown(EKeys::D))
	{
		Right += 1.0f;
	}
	if (PC->IsInputKeyDown(EKeys::A))
	{
		Right -= 1.0f;
	}
	MoveForward(Forward);
	MoveRight(Right);

	float MouseX = 0.0f;
	float MouseY = 0.0f;
	PC->GetInputMouseDelta(MouseX, MouseY);
	AddControllerYawInput(MouseX);
	AddControllerPitchInput(-MouseY);

	if (PC->WasInputKeyJustPressed(EKeys::E))
	{
		OnInteractPressed();
	}
	if (PC->WasInputKeyJustPressed(EKeys::One))
	{
		OnStationPrimaryAction();
	}

	if (AActor* Focus = FocusedInteractable.Get())
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				7101,
				0.0f,
				FColor::Cyan,
				ICrewInteractable::Execute_GetInteractPrompt(Focus, this).ToString());
		}
	}

	(void)DeltaSeconds;
}

void ASpaceshipCrewCharacter::MoveForward(float Value)
{
	if (Controller && !FMath::IsNearlyZero(Value))
	{
		const FRotator YawRot(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
		AddMovementInput(FRotationMatrix(YawRot).GetUnitAxis(EAxis::X), Value);
	}
}

void ASpaceshipCrewCharacter::MoveRight(float Value)
{
	if (Controller && !FMath::IsNearlyZero(Value))
	{
		const FRotator YawRot(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
		AddMovementInput(FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y), Value);
	}
}

void ASpaceshipCrewCharacter::TurnAtRate(float Rate)
{
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void ASpaceshipCrewCharacter::LookUpAtRate(float Rate)
{
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void ASpaceshipCrewCharacter::UpdateFocusedInteractable()
{
	FocusedInteractable = nullptr;

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector Start = FollowCamera ? FollowCamera->GetComponentLocation() : GetActorLocation() + FVector(0, 0, 60);
	const FVector End = Start + (FollowCamera ? FollowCamera->GetForwardVector() : GetActorForwardVector()) * InteractTraceDistance;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(CrewInteract), false, this);
	FHitResult Hit;
	const bool bHit = World->SweepSingleByChannel(
		Hit,
		Start,
		End,
		FQuat::Identity,
		ECC_Visibility,
		FCollisionShape::MakeSphere(InteractTraceRadius),
		Params);

	if (!bHit || !Hit.GetActor())
	{
		return;
	}

	AActor* Candidate = Hit.GetActor();
	if (Candidate->GetClass()->ImplementsInterface(UCrewInteractable::StaticClass()))
	{
		if (ICrewInteractable::Execute_CanInteract(Candidate, this))
		{
			FocusedInteractable = Candidate;
		}
	}
}

void ASpaceshipCrewCharacter::OnInteractPressed()
{
	if (ACrewWorkstation* Station = OccupiedWorkstation.Get())
	{
		if (Station->GetOperator() == this)
		{
			Station->Leave();
			OccupiedWorkstation = nullptr;
			return;
		}
	}

	UpdateFocusedInteractable();
	if (AActor* Target = FocusedInteractable.Get())
	{
		ICrewInteractable::Execute_Interact(Target, this);
		if (ACrewWorkstation* Occupied = Cast<ACrewWorkstation>(Target))
		{
			OccupiedWorkstation = Occupied->IsOccupied() && Occupied->GetOperator() == this ? Occupied : nullptr;
		}
	}
}

void ASpaceshipCrewCharacter::OnStationPrimaryAction()
{
	ACrewWorkstation* Station = OccupiedWorkstation.Get();
	if (!Station || Station->GetOperator() != this)
	{
		return;
	}
	if (AEngineerEnergyConsole* Energy = Cast<AEngineerEnergyConsole>(Station))
	{
		Energy->ApplyDefensePreset();
	}
}
