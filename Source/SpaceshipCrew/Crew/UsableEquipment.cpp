#include "UsableEquipment.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Blueprint/UserWidget.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

AUsableEquipment::AUsableEquipment()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	DisplayMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DisplayMesh"));
	DisplayMesh->SetupAttachment(SceneRoot);
	DisplayMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));

	AnimatedMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("AnimatedMesh"));
	AnimatedMesh->SetupAttachment(SceneRoot);
	AnimatedMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AnimatedMesh->SetVisibility(false);
	AnimatedMesh->SetHiddenInGame(true);

	InteractionVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionVolume"));
	InteractionVolume->SetupAttachment(SceneRoot);
	InteractionVolume->SetBoxExtent(FVector(70.0f, 70.0f, 90.0f));
	InteractionVolume->SetRelativeLocation(FVector(90.0f, 0.0f, 40.0f));
	InteractionVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionVolume->SetCollisionObjectType(ECC_WorldDynamic);
	InteractionVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	InteractionVolume->SetGenerateOverlapEvents(true);
	InteractionVolume->SetHiddenInGame(true);

	PromptAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("PromptAnchor"));
	PromptAnchor->SetupAttachment(DisplayMesh);

	UseAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("UseAnchor"));
	UseAnchor->SetupAttachment(SceneRoot);
	UseAnchor->SetRelativeLocation(FVector(80.0f, 0.0f, 0.0f));

	PromptText = NSLOCTEXT("SpaceshipCrew", "Equipment_Use", "Использовать");
}

void AUsableEquipment::BeginPlay()
{
	Super::BeginPlay();

	if (AnimatedMesh && AnimatedMesh->GetSkeletalMeshAsset())
	{
		AnimatedMesh->SetVisibility(true);
		AnimatedMesh->SetHiddenInGame(false);
	}
}

FVector AUsableEquipment::GetPromptAnchorWorldLocation() const
{
	if (PromptAnchor)
	{
		return PromptAnchor->GetComponentLocation();
	}
	if (DisplayMesh)
	{
		return DisplayMesh->GetComponentLocation();
	}
	return GetActorLocation();
}

bool AUsableEquipment::CanInteract_Implementation(APawn* InstigatorPawn) const
{
	if (!InstigatorPawn)
	{
		return false;
	}
	if (!UserPawn)
	{
		return true;
	}
	return UserPawn == InstigatorPawn;
}

void AUsableEquipment::Interact_Implementation(APawn* InstigatorPawn)
{
	if (!InstigatorPawn)
	{
		return;
	}
	if (UserPawn == InstigatorPawn)
	{
		EndUse();
		return;
	}
	BeginUse(InstigatorPawn);
}

FText AUsableEquipment::GetInteractPrompt_Implementation(APawn* InstigatorPawn) const
{
	if (UserPawn && UserPawn == InstigatorPawn)
	{
		if (UseMode == EUsableEquipmentUseMode::Interact)
		{
			return NSLOCTEXT("SpaceshipCrew", "Equipment_Close", "Закрыть");
		}
		return NSLOCTEXT("SpaceshipCrew", "Equipment_Leave", "Встать");
	}
	return PromptText.IsEmpty()
		? NSLOCTEXT("SpaceshipCrew", "Equipment_Use", "Использовать")
		: PromptText;
}

bool AUsableEquipment::IsMovementLocked() const
{
	return UserPawn
		&& (UseMode == EUsableEquipmentUseMode::Occupy
			|| UseMode == EUsableEquipmentUseMode::OccupyRedirectInput);
}

bool AUsableEquipment::WantsMovementRedirect() const
{
	return UserPawn && UseMode == EUsableEquipmentUseMode::OccupyRedirectInput;
}

void AUsableEquipment::NotifyRedirectedMove(const FVector2D MoveInput)
{
	if (WantsMovementRedirect())
	{
		OnRedirectedMove(MoveInput);
	}
}

bool AUsableEquipment::BeginUse(APawn* User)
{
	if (!User || UserPawn)
	{
		return false;
	}

	UserPawn = User;

	if (IsMovementLocked() && UseAnchor)
	{
		if (DisplayMesh)
		{
			DisplayMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		}
		User->SetActorLocation(UseAnchor->GetComponentLocation());
		User->SetActorRotation(UseAnchor->GetComponentRotation());
		SetUserMovementLocked(User, true);
	}

	PlayUseAnimations(User);
	ShowInteractionUI(User);
	OnEquipmentUsed(User);
	return true;
}

void AUsableEquipment::EndUse()
{
	if (!UserPawn)
	{
		return;
	}

	APawn* PreviousUser = UserPawn;
	const bool bWasLocked = IsMovementLocked();

	if (ACharacter* Character = Cast<ACharacter>(PreviousUser))
	{
		if (USkeletalMeshComponent* Mesh = Character->GetMesh())
		{
			if (UAnimInstance* CharacterAnim = Mesh->GetAnimInstance())
			{
				if (CharacterUseMontage)
				{
					CharacterAnim->Montage_Stop(0.2f, CharacterUseMontage);
				}
			}
		}
	}

	if (bWasLocked && UseAnchor)
	{
		const FVector StandLocation = UseAnchor->GetComponentTransform().TransformPosition(ReleaseOffset);
		PreviousUser->SetActorLocation(StandLocation);
		PreviousUser->SetActorRotation(UseAnchor->GetComponentRotation());
	}

	if (bWasLocked)
	{
		if (DisplayMesh)
		{
			DisplayMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
		}
		SetUserMovementLocked(PreviousUser, false);
	}

	HideInteractionUI();
	UserPawn = nullptr;
	OnEquipmentReleased(PreviousUser);
}

void AUsableEquipment::PlayUseAnimations(APawn* User)
{
	if (EquipmentUseMontage && AnimatedMesh && AnimatedMesh->GetSkeletalMeshAsset())
	{
		if (UAnimInstance* EquipmentAnim = AnimatedMesh->GetAnimInstance())
		{
			EquipmentAnim->Montage_Play(EquipmentUseMontage);
		}
	}

	if (!CharacterUseMontage)
	{
		return;
	}

	if (ACharacter* Character = Cast<ACharacter>(User))
	{
		if (USkeletalMeshComponent* Mesh = Character->GetMesh())
		{
			if (UAnimInstance* CharacterAnim = Mesh->GetAnimInstance())
			{
				CharacterAnim->Montage_Play(CharacterUseMontage);
			}
		}
	}
}

void AUsableEquipment::ShowInteractionUI(APawn* User)
{
	if (!InteractionWidgetClass || !User)
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(User->GetController());
	if (!PC)
	{
		return;
	}

	ActiveWidget = CreateWidget<UUserWidget>(PC, InteractionWidgetClass);
	if (ActiveWidget)
	{
		ActiveWidget->AddToViewport(20);
	}
}

void AUsableEquipment::HideInteractionUI()
{
	if (ActiveWidget)
	{
		ActiveWidget->RemoveFromParent();
		ActiveWidget = nullptr;
	}
}

void AUsableEquipment::SetUserMovementLocked(APawn* User, const bool bLocked) const
{
	ACharacter* Character = Cast<ACharacter>(User);
	if (!Character)
	{
		return;
	}

	if (UCharacterMovementComponent* Move = Character->GetCharacterMovement())
	{
		if (bLocked)
		{
			Move->DisableMovement();
		}
		else
		{
			Move->SetMovementMode(MOVE_Walking);
		}
	}
}
