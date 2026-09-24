#include "EquippableItem.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Blueprint/UserWidget.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InventoryComponent.h"
#include "SpaceshipCrewCharacter.h"

AEquippableItem::AEquippableItem()
{
	PrimaryActorTick.bCanEverTick = false;

	// DisplayMesh — root, иначе SimulatePhysics/гравитация не работают.
	DisplayMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DisplayMesh"));
	SetRootComponent(DisplayMesh);
	DisplayMesh->SetCollisionProfileName(TEXT("PhysicsActor"));
	DisplayMesh->SetSimulatePhysics(false);
	DisplayMesh->SetEnableGravity(true);
	DisplayMesh->SetNotifyRigidBodyCollision(false);

	SceneRoot = DisplayMesh;

	AnimatedMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("AnimatedMesh"));
	AnimatedMesh->SetupAttachment(DisplayMesh);
	AnimatedMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AnimatedMesh->SetVisibility(false);
	AnimatedMesh->SetHiddenInGame(true);

	InteractionVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionVolume"));
	InteractionVolume->SetupAttachment(DisplayMesh);
	InteractionVolume->SetBoxExtent(FVector(50.0f, 50.0f, 60.0f));
	InteractionVolume->SetRelativeLocation(FVector(0.0f, 0.0f, 40.0f));
	InteractionVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionVolume->SetCollisionObjectType(ECC_WorldDynamic);
	InteractionVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	InteractionVolume->SetGenerateOverlapEvents(true);
	InteractionVolume->SetHiddenInGame(true);

	PromptAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("PromptAnchor"));
	PromptAnchor->SetupAttachment(DisplayMesh);

	PromptText = NSLOCTEXT("SpaceshipCrew", "Item_Pickup", "Взять");
}

void AEquippableItem::BeginPlay()
{
	Super::BeginPlay();

	if (AnimatedMesh && AnimatedMesh->GetSkeletalMeshAsset())
	{
		AnimatedMesh->SetVisibility(true);
		AnimatedMesh->SetHiddenInGame(false);
	}

	if (bIsWorldPickup)
	{
		EnableWorldPhysics();
	}
}

bool AEquippableItem::CanInteract_Implementation(APawn* InstigatorPawn) const
{
	if (!bIsWorldPickup || !InstigatorPawn)
	{
		return false;
	}

	if (const ASpaceshipCrewCharacter* Character = Cast<ASpaceshipCrewCharacter>(InstigatorPawn))
	{
		if (Character->IsMovementLockedByEquipment())
		{
			return false;
		}
	}

	const UInventoryComponent* Inventory = InstigatorPawn->FindComponentByClass<UInventoryComponent>();
	if (!Inventory)
	{
		return false;
	}

	for (const FInventoryEntry& Entry : Inventory->GetSlots())
	{
		if (!Entry.ItemClass)
		{
			return true;
		}
	}
	return Inventory->GetSlots().Num() < Inventory->GetCapacity();
}

void AEquippableItem::Interact_Implementation(APawn* InstigatorPawn)
{
	if (!InstigatorPawn || !bIsWorldPickup)
	{
		return;
	}

	UInventoryComponent* Inventory = InstigatorPawn->FindComponentByClass<UInventoryComponent>();
	if (!Inventory)
	{
		return;
	}

	int32 SlotIndex = INDEX_NONE;
	if (!Inventory->TryAddItem(GetClass(), SlotIndex))
	{
		return;
	}

	// Сразу на первый свободный слот хотбара — предмет видно без открытия инвентаря.
	const TArray<int32>& Hotbar = Inventory->GetHotbarSlotIndices();
	for (int32 HotbarIndex = 0; HotbarIndex < Hotbar.Num(); ++HotbarIndex)
	{
		if (Hotbar[HotbarIndex] == INDEX_NONE)
		{
			Inventory->AssignToHotbar(SlotIndex, HotbarIndex);
			break;
		}
	}

	OnPickedUp(InstigatorPawn);
	bIsWorldPickup = false;
	Destroy();
}

FText AEquippableItem::GetInteractPrompt_Implementation(APawn* InstigatorPawn) const
{
	(void)InstigatorPawn;
	return PromptText.IsEmpty()
		? NSLOCTEXT("SpaceshipCrew", "Item_Pickup", "Взять")
		: PromptText;
}

FText AEquippableItem::GetInteractDisplayName_Implementation(APawn* InstigatorPawn) const
{
	(void)InstigatorPawn;
	return DisplayName;
}

FVector AEquippableItem::GetPromptAnchorWorldLocation_Implementation() const
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

UPrimitiveComponent* AEquippableItem::GetInteractionPrimitive_Implementation() const
{
	return InteractionVolume;
}

void AEquippableItem::ConfigureAsHeldVisual()
{
	bIsWorldPickup = false;
	DisableWorldPhysics();
	SetActorEnableCollision(false);
	if (InteractionVolume)
	{
		InteractionVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		InteractionVolume->SetGenerateOverlapEvents(false);
	}
	if (DisplayMesh)
	{
		DisplayMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void AEquippableItem::ConfigureAsWorldPickup()
{
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	bIsWorldPickup = true;
	SetActorEnableCollision(true);
	if (DisplayMesh)
	{
		DisplayMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		DisplayMesh->SetCollisionProfileName(TEXT("PhysicsActor"));
	}
	if (InteractionVolume)
	{
		InteractionVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		InteractionVolume->SetCollisionObjectType(ECC_WorldDynamic);
		InteractionVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
		InteractionVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
		InteractionVolume->SetGenerateOverlapEvents(true);
	}
	HideHeldUI();
	EnableWorldPhysics();
}

void AEquippableItem::EnableWorldPhysics()
{
	if (!DisplayMesh)
	{
		return;
	}
	DisplayMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	DisplayMesh->SetCollisionProfileName(TEXT("PhysicsActor"));
	DisplayMesh->SetSimulatePhysics(true);
	DisplayMesh->SetEnableGravity(true);
	DisplayMesh->WakeRigidBody();
}

void AEquippableItem::DisableWorldPhysics()
{
	if (!DisplayMesh)
	{
		return;
	}
	DisplayMesh->SetSimulatePhysics(false);
	DisplayMesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
	DisplayMesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
}

void AEquippableItem::AttachToCharacterHand(APawn* OwnerPawn)
{
	ACharacter* Character = Cast<ACharacter>(OwnerPawn);
	if (!Character || !Character->GetMesh())
	{
		return;
	}

	ConfigureAsHeldVisual();
	AttachToComponent(
		Character->GetMesh(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		HandSocketName);
	SetActorRelativeLocation(HeldRelativeLocation);
	SetActorRelativeRotation(HeldRelativeRotation);
}

void AEquippableItem::OnPickedUp_Implementation(APawn* Picker)
{
	(void)Picker;
}

void AEquippableItem::OnDropped_Implementation(APawn* Dropper)
{
	(void)Dropper;
}

void AEquippableItem::OnEquipped_Implementation(APawn* User)
{
	PlayCharacterMontage(User, CharacterEquipMontage);
	ShowHeldUI(User);
}

void AEquippableItem::OnUnequipped_Implementation(APawn* User)
{
	(void)User;
	HideHeldUI();
}

bool AEquippableItem::CanPrimaryUse_Implementation(APawn* User) const
{
	return User != nullptr;
}

void AEquippableItem::OnPrimaryUse_Implementation(APawn* User)
{
	PlayCharacterMontage(User, CharacterUseMontage);
}

void AEquippableItem::OnSecondaryUse_Implementation(APawn* User)
{
	(void)User;
}

void AEquippableItem::ShowHeldUI(APawn* User)
{
	if (!HeldWidgetClass || !User)
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(User->GetController());
	if (!PC)
	{
		return;
	}

	HideHeldUI();
	ActiveHeldWidget = CreateWidget<UUserWidget>(PC, HeldWidgetClass);
	if (ActiveHeldWidget)
	{
		ActiveHeldWidget->AddToViewport(15);
	}
}

void AEquippableItem::HideHeldUI()
{
	if (ActiveHeldWidget)
	{
		ActiveHeldWidget->RemoveFromParent();
		ActiveHeldWidget = nullptr;
	}
}

void AEquippableItem::PlayCharacterMontage(APawn* User, UAnimMontage* Montage) const
{
	if (!Montage)
	{
		return;
	}

	if (ACharacter* Character = Cast<ACharacter>(User))
	{
		if (USkeletalMeshComponent* Mesh = Character->GetMesh())
		{
			if (UAnimInstance* CharacterAnim = Mesh->GetAnimInstance())
			{
				CharacterAnim->Montage_Play(Montage);
			}
		}
	}
}
