#include "SpaceshipCrewCharacter.h"

#include "UsableEquipment.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "CrewInteractable.h"
#include "Engine/OverlapResult.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputCoreTypes.h"
#include "UObject/ConstructorHelpers.h"
#include "UI/UsableEquipmentPromptWidget.h"

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

	PromptWidgetClass = UUsableEquipmentPromptWidget::StaticClass();

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
	RefreshInteractPrompt();
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

	AUsableEquipment* Active = ActiveEquipment.Get();
	const bool bUsing = Active && Active->IsInUseBy(this);
	if (bUsing && Active->WantsMovementRedirect())
	{
		Active->NotifyRedirectedMove(FVector2D(Right, Forward));
	}
	else if (!bUsing || !Active->IsMovementLocked())
	{
		MoveForward(Forward);
		MoveRight(Right);
	}

	float MouseX = 0.0f;
	float MouseY = 0.0f;
	PC->GetInputMouseDelta(MouseX, MouseY);
	AddControllerYawInput(MouseX);
	AddControllerPitchInput(-MouseY);

	if (PC->WasInputKeyJustPressed(EKeys::E))
	{
		OnInteractPressed();
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
	UCapsuleComponent* Capsule = GetCapsuleComponent();
	if (!World || !Capsule)
	{
		return;
	}

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(UsableEquipmentFocus), false, this);
	const FCollisionShape Shape = FCollisionShape::MakeCapsule(
		Capsule->GetScaledCapsuleRadius(),
		Capsule->GetScaledCapsuleHalfHeight());
	World->OverlapMultiByChannel(
		Overlaps,
		Capsule->GetComponentLocation(),
		Capsule->GetComponentQuat(),
		ECC_Pawn,
		Shape,
		Params);

	AUsableEquipment* Best = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AUsableEquipment* Equipment = Cast<AUsableEquipment>(Overlap.GetActor());
		if (!Equipment || Overlap.GetComponent() != Equipment->GetInteractionVolume())
		{
			continue;
		}
		if (!ICrewInteractable::Execute_CanInteract(Equipment, this))
		{
			continue;
		}
		const float DistSq = FVector::DistSquared(
			GetActorLocation(),
			Equipment->GetInteractionVolume()->GetComponentLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Best = Equipment;
		}
	}

	FocusedInteractable = Best;
}

void ASpaceshipCrewCharacter::EnsurePromptWidget()
{
	if (PromptWidget)
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return;
	}

	const TSubclassOf<UUsableEquipmentPromptWidget> WidgetClass = PromptWidgetClass
		? PromptWidgetClass
		: TSubclassOf<UUsableEquipmentPromptWidget>(UUsableEquipmentPromptWidget::StaticClass());
	PromptWidget = CreateWidget<UUsableEquipmentPromptWidget>(PC, WidgetClass);
	if (PromptWidget)
	{
		PromptWidget->AddToViewport(5);
		PromptWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void ASpaceshipCrewCharacter::RefreshInteractPrompt()
{
	EnsurePromptWidget();
	if (!PromptWidget)
	{
		return;
	}

	AActor* Focus = FocusedInteractable.Get();
	AUsableEquipment* Equipment = Cast<AUsableEquipment>(Focus);
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!Focus || !Equipment || !PC)
	{
		PromptWidget->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	const FVector AnchorWorld = Equipment->GetPromptAnchorWorldLocation();
	FVector2D Pixel = FVector2D::ZeroVector;
	const bool bProjected = PC->ProjectWorldLocationToScreen(AnchorWorld, Pixel, true);
	const float ViewportScale = FMath::Max(UWidgetLayoutLibrary::GetViewportScale(PromptWidget), 0.01f);

	const FText Action = ICrewInteractable::Execute_GetInteractPrompt(Focus, this);
	const FText Title = Equipment->DisplayName.IsEmpty() ? Action : Equipment->DisplayName;
	const bool bShowAction = !Equipment->DisplayName.IsEmpty() && !Title.EqualTo(Action);

	PromptWidget->SetCallout(Title, Action, Pixel / ViewportScale, bProjected, bShowAction);
	PromptWidget->SetVisibility(bProjected ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

void ASpaceshipCrewCharacter::OnInteractPressed()
{
	if (AUsableEquipment* Active = ActiveEquipment.Get())
	{
		if (Active->IsInUseBy(this))
		{
			Active->EndUse();
			ActiveEquipment = nullptr;
			return;
		}
		ActiveEquipment = nullptr;
	}

	UpdateFocusedInteractable();
	if (AActor* Target = FocusedInteractable.Get())
	{
		ICrewInteractable::Execute_Interact(Target, this);
		if (AUsableEquipment* Equipment = Cast<AUsableEquipment>(Target))
		{
			ActiveEquipment = Equipment->IsInUseBy(this) ? Equipment : nullptr;
		}
	}
}
