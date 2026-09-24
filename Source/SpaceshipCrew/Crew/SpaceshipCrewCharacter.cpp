#include "SpaceshipCrewCharacter.h"

#include "UsableEquipment.h"
#include "EquippableItem.h"
#include "InventoryComponent.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
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
#include "UI/CrewHotbarWidget.h"
#include "UI/CrewInventoryWidget.h"

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
	CameraBoom->SetRelativeLocation(CameraBoomLocation);
	CameraBoom->TargetArmLength = CameraArmLength;
	CameraBoom->SocketOffset = CameraSocketOffset;
	CameraBoom->bDoCollisionTest = true;
	CameraBoom->ProbeSize = 28.0f;
	CameraBoom->ProbeChannel = ECC_Camera;
	CameraBoom->bUsePawnControlRotation = true;
	// Плавное приближение только когда probe упирается в потолок/стену.
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = CameraCollisionLagSpeed;
	CameraBoom->bEnableCameraRotationLag = false;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	Inventory = CreateDefaultSubobject<UInventoryComponent>(TEXT("Inventory"));

	PromptWidgetClass = UUsableEquipmentPromptWidget::StaticClass();
	HotbarWidgetClass = UCrewHotbarWidget::StaticClass();
	InventoryWidgetClass = UCrewInventoryWidget::StaticClass();

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

	EnsureInventoryWidgets();
	ApplyCameraPitchLimits();
}

void ASpaceshipCrewCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	EnsureInventoryWidgets();
	ApplyCameraPitchLimits();
}

void ASpaceshipCrewCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	(void)PlayerInputComponent;
	// Enhanced Input — дефолт проекта; legacy BindAxis на нём ненадёжен.
	// Игровой ввод читаем в Tick через APlayerController (WASD / мышь / E / I / 1-5).
}

void ASpaceshipCrewCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	PollGameplayInput(DeltaSeconds);
	ClampControlPitch();
	UpdateFocusedInteractable();
	RefreshInteractPrompt();
}

float ASpaceshipCrewCharacter::NormalizePitchDegrees(float Pitch)
{
	while (Pitch > 180.0f)
	{
		Pitch -= 360.0f;
	}
	while (Pitch < -180.0f)
	{
		Pitch += 360.0f;
	}
	return Pitch;
}

void ASpaceshipCrewCharacter::ApplyCameraPitchLimits()
{
	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		if (PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->ViewPitchMin = CameraLookUpPitchMin;
			PC->PlayerCameraManager->ViewPitchMax = CameraLookDownPitchMax;
		}
	}

	if (CameraBoom)
	{
		CameraBoom->TargetArmLength = CameraArmLength;
		CameraBoom->SocketOffset = CameraSocketOffset;
		CameraBoom->SetRelativeLocation(CameraBoomLocation);
		CameraBoom->bDoCollisionTest = true;
		CameraBoom->bEnableCameraLag = true;
		CameraBoom->CameraLagSpeed = CameraCollisionLagSpeed;
	}
}

void ASpaceshipCrewCharacter::ClampControlPitch()
{
	if (!Controller)
	{
		return;
	}

	const float Pitch = NormalizePitchDegrees(Controller->GetControlRotation().Pitch);
	const float ClampedPitch = FMath::Clamp(Pitch, CameraLookUpPitchMin, CameraLookDownPitchMax);
	if (!FMath::IsNearlyEqual(Pitch, ClampedPitch, 0.01f))
	{
		FRotator ControlRot = Controller->GetControlRotation();
		ControlRot.Pitch = ClampedPitch;
		Controller->SetControlRotation(ControlRot);
	}
}

bool ASpaceshipCrewCharacter::IsMovementLockedByEquipment() const
{
	const AUsableEquipment* Active = ActiveEquipment.Get();
	return Active && Active->IsInUseBy(this) && Active->IsMovementLocked();
}

void ASpaceshipCrewCharacter::PollGameplayInput(float DeltaSeconds)
{
	APlayerController* PC = Cast<APlayerController>(Controller);
	if (!PC)
	{
		return;
	}

	PollHotbarAndInventory(PC, DeltaSeconds);

	const bool bInventoryOpen = Inventory && Inventory->IsInventoryOpen();
	if (!bInventoryOpen)
	{
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

		PollHeldItemUse(PC);
	}

	if (PC->WasInputKeyJustPressed(EKeys::E) && !bInventoryOpen)
	{
		OnInteractPressed();
	}
}

void ASpaceshipCrewCharacter::PollHotbarAndInventory(APlayerController* PC, const float DeltaSeconds)
{
	if (!PC || !Inventory)
	{
		return;
	}

	if (PC->WasInputKeyJustPressed(EKeys::I))
	{
		Inventory->ToggleInventory();
	}

	static const FKey HotbarKeys[5] = {
		EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five};
	for (int32 Index = 0; Index < 5; ++Index)
	{
		if (PC->WasInputKeyJustPressed(HotbarKeys[Index]))
		{
			Inventory->ActivateHotbarSlot(Index);
		}
	}

	if (PC->IsInputKeyDown(EKeys::R) && !Inventory->IsInventoryOpen())
	{
		ClearActiveHoldTime += DeltaSeconds;
		if (!bClearActiveTriggered && ClearActiveHoldTime >= ClearActiveHoldSeconds)
		{
			Inventory->ClearActiveSlot();
			bClearActiveTriggered = true;
		}
	}
	else
	{
		ClearActiveHoldTime = 0.0f;
		bClearActiveTriggered = false;
	}
}

void ASpaceshipCrewCharacter::PollHeldItemUse(APlayerController* PC)
{
	if (!PC || !Inventory || Inventory->IsInventoryOpen())
	{
		return;
	}

	AEquippableItem* Held = Inventory->GetHeldItem();
	if (!Held)
	{
		return;
	}

	if (PC->WasInputKeyJustPressed(EKeys::LeftMouseButton))
	{
		if (Held->CanPrimaryUse(this))
		{
			Held->OnPrimaryUse(this);
		}
	}

	if (PC->WasInputKeyJustPressed(EKeys::RightMouseButton) && Held->SupportsSecondaryAction())
	{
		Held->OnSecondaryUse(this);
	}
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

	if (IsMovementLockedByEquipment())
	{
		if (AUsableEquipment* Active = ActiveEquipment.Get())
		{
			FocusedInteractable = Active;
		}
		return;
	}

	UWorld* World = GetWorld();
	UCapsuleComponent* Capsule = GetCapsuleComponent();
	if (!World || !Capsule)
	{
		return;
	}

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(CrewInteractFocus), false, this);
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

	AActor* Best = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Candidate = Overlap.GetActor();
		if (!Candidate || !Candidate->Implements<UCrewInteractable>())
		{
			continue;
		}

		UPrimitiveComponent* InteractionPrim =
			ICrewInteractable::Execute_GetInteractionPrimitive(Candidate);
		if (!InteractionPrim || Overlap.GetComponent() != InteractionPrim)
		{
			continue;
		}
		if (!ICrewInteractable::Execute_CanInteract(Candidate, this))
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(
			GetActorLocation(),
			InteractionPrim->GetComponentLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Best = Candidate;
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

void ASpaceshipCrewCharacter::EnsureInventoryWidgets()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC || !Inventory)
	{
		return;
	}

	if (!HotbarWidget)
	{
		const TSubclassOf<UCrewHotbarWidget> WidgetClass = HotbarWidgetClass
			? HotbarWidgetClass
			: TSubclassOf<UCrewHotbarWidget>(UCrewHotbarWidget::StaticClass());
		HotbarWidget = CreateWidget<UCrewHotbarWidget>(PC, WidgetClass);
		if (HotbarWidget)
		{
			HotbarWidget->AddToViewport(40);
			HotbarWidget->BindInventory(Inventory);
		}
	}

	if (!InventoryWidget)
	{
		const TSubclassOf<UCrewInventoryWidget> WidgetClass = InventoryWidgetClass
			? InventoryWidgetClass
			: TSubclassOf<UCrewInventoryWidget>(UCrewInventoryWidget::StaticClass());
		InventoryWidget = CreateWidget<UCrewInventoryWidget>(PC, WidgetClass);
		if (InventoryWidget)
		{
			InventoryWidget->AddToViewport(25);
			InventoryWidget->BindInventory(Inventory);
		}
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
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!Focus || !PC || !Focus->Implements<UCrewInteractable>())
	{
		PromptWidget->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	const FVector AnchorWorld = ICrewInteractable::Execute_GetPromptAnchorWorldLocation(Focus);
	FVector2D Pixel = FVector2D::ZeroVector;
	const bool bProjected = PC->ProjectWorldLocationToScreen(AnchorWorld, Pixel, true);
	const float ViewportScale = FMath::Max(UWidgetLayoutLibrary::GetViewportScale(PromptWidget), 0.01f);

	const FText Action = ICrewInteractable::Execute_GetInteractPrompt(Focus, this);
	const FText Display = ICrewInteractable::Execute_GetInteractDisplayName(Focus, this);
	const FText Title = Display.IsEmpty() ? Action : Display;
	const bool bShowAction = !Display.IsEmpty() && !Title.EqualTo(Action);

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
