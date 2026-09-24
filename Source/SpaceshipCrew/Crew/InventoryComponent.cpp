#include "InventoryComponent.h"

#include "EquippableItem.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	Slots.SetNum(Capacity);
	EnsureHotbarSize();
}

void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	if (Slots.Num() != Capacity)
	{
		Slots.SetNum(Capacity);
	}
	EnsureHotbarSize();
}

void UInventoryComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnequipHeldInternal();
	Super::EndPlay(EndPlayReason);
}

void UInventoryComponent::EnsureHotbarSize()
{
	const int32 OldNum = HotbarSlotIndices.Num();
	HotbarSlotIndices.SetNum(HotbarSize);
	for (int32 Index = OldNum; Index < HotbarSize; ++Index)
	{
		HotbarSlotIndices[Index] = INDEX_NONE;
	}
	if (ActiveHotbarIndex != INDEX_NONE && ActiveHotbarIndex >= HotbarSize)
	{
		ClearActiveSlot();
	}
}

void UInventoryComponent::SetCapacity(const int32 NewCapacity)
{
	Capacity = FMath::Max(1, NewCapacity);
	const int32 OldNum = Slots.Num();
	Slots.SetNum(Capacity);
	for (int32 Index = OldNum; Index < Capacity; ++Index)
	{
		Slots[Index] = FInventoryEntry();
	}

	for (int32& HotbarIndex : HotbarSlotIndices)
	{
		if (HotbarIndex != INDEX_NONE && HotbarIndex >= Capacity)
		{
			HotbarIndex = INDEX_NONE;
		}
	}

	OnInventoryChanged.Broadcast();
	OnHotbarChanged.Broadcast();
}

bool UInventoryComponent::TryAddItem(const TSubclassOf<AEquippableItem> ItemClass, int32& OutSlotIndex)
{
	OutSlotIndex = INDEX_NONE;
	if (!ItemClass)
	{
		return false;
	}

	if (Slots.Num() < Capacity)
	{
		Slots.SetNum(Capacity);
	}

	for (int32 Index = 0; Index < Slots.Num(); ++Index)
	{
		if (!Slots[Index].ItemClass)
		{
			Slots[Index].ItemClass = ItemClass;
			OutSlotIndex = Index;
			OnInventoryChanged.Broadcast();
			return true;
		}
	}

	return false;
}

bool UInventoryComponent::RemoveAt(const int32 SlotIndex)
{
	if (!Slots.IsValidIndex(SlotIndex) || !Slots[SlotIndex].ItemClass)
	{
		return false;
	}

	if (HeldItem.IsValid())
	{
		for (int32 HotbarIndex = 0; HotbarIndex < HotbarSlotIndices.Num(); ++HotbarIndex)
		{
			if (HotbarSlotIndices[HotbarIndex] == SlotIndex && ActiveHotbarIndex == HotbarIndex)
			{
				ClearActiveSlot();
				break;
			}
		}
	}

	Slots[SlotIndex] = FInventoryEntry();
	for (int32& HotbarIndex : HotbarSlotIndices)
	{
		if (HotbarIndex == SlotIndex)
		{
			HotbarIndex = INDEX_NONE;
		}
	}

	OnInventoryChanged.Broadcast();
	OnHotbarChanged.Broadcast();
	return true;
}

bool UInventoryComponent::AssignToHotbar(const int32 InventorySlotIndex, const int32 HotbarIndex)
{
	EnsureHotbarSize();
	if (!HotbarSlotIndices.IsValidIndex(HotbarIndex))
	{
		return false;
	}
	if (InventorySlotIndex != INDEX_NONE
		&& (!Slots.IsValidIndex(InventorySlotIndex) || !Slots[InventorySlotIndex].ItemClass))
	{
		return false;
	}

	HotbarSlotIndices[HotbarIndex] = InventorySlotIndex;
	OnHotbarChanged.Broadcast();

	if (ActiveHotbarIndex == HotbarIndex)
	{
		if (InventorySlotIndex == INDEX_NONE)
		{
			ClearActiveSlot();
		}
		else
		{
			ActivateHotbarSlot(HotbarIndex);
		}
	}
	return true;
}

bool UInventoryComponent::ClearHotbarSlot(const int32 HotbarIndex)
{
	return AssignToHotbar(INDEX_NONE, HotbarIndex);
}

bool UInventoryComponent::SwapSlots(const int32 SlotA, const int32 SlotB)
{
	if (SlotA == SlotB || !Slots.IsValidIndex(SlotA) || !Slots.IsValidIndex(SlotB))
	{
		return false;
	}

	Slots.Swap(SlotA, SlotB);

	for (int32& HotbarIndex : HotbarSlotIndices)
	{
		if (HotbarIndex == SlotA)
		{
			HotbarIndex = SlotB;
		}
		else if (HotbarIndex == SlotB)
		{
			HotbarIndex = SlotA;
		}
	}

	OnInventoryChanged.Broadcast();
	OnHotbarChanged.Broadcast();
	return true;
}

bool UInventoryComponent::ActivateHotbarSlot(const int32 HotbarIndex)
{
	EnsureHotbarSize();
	if (!HotbarSlotIndices.IsValidIndex(HotbarIndex))
	{
		return false;
	}

	const int32 InventoryIndex = HotbarSlotIndices[HotbarIndex];
	if (InventoryIndex == INDEX_NONE || !Slots.IsValidIndex(InventoryIndex) || !Slots[InventoryIndex].ItemClass)
	{
		return false;
	}

	if (ActiveHotbarIndex == HotbarIndex && HeldItem.IsValid())
	{
		// Повторное нажатие той же клавиши — убрать из рук.
		ClearActiveSlot();
		return true;
	}

	UnequipHeldInternal();
	AEquippableItem* Visual = SpawnHeldVisual(Slots[InventoryIndex].ItemClass);
	if (!Visual)
	{
		return false;
	}

	HeldItem = Visual;
	ActiveHotbarIndex = HotbarIndex;
	Visual->AttachToCharacterHand(GetOwnerPawn());
	Visual->OnEquipped(GetOwnerPawn());
	OnHotbarChanged.Broadcast();
	OnHeldItemChanged.Broadcast();
	return true;
}

void UInventoryComponent::ClearActiveSlot()
{
	const bool bHadActive = ActiveHotbarIndex != INDEX_NONE || HeldItem.IsValid();
	UnequipHeldInternal();
	ActiveHotbarIndex = INDEX_NONE;
	if (bHadActive)
	{
		OnHotbarChanged.Broadcast();
		OnHeldItemChanged.Broadcast();
	}
}

bool UInventoryComponent::EquipFromInventoryIndex(const int32 InventorySlotIndex)
{
	if (!Slots.IsValidIndex(InventorySlotIndex) || !Slots[InventorySlotIndex].ItemClass)
	{
		return false;
	}

	EnsureHotbarSize();
	int32 HotbarIndex = INDEX_NONE;
	for (int32 Index = 0; Index < HotbarSlotIndices.Num(); ++Index)
	{
		if (HotbarSlotIndices[Index] == InventorySlotIndex)
		{
			HotbarIndex = Index;
			break;
		}
	}

	if (HotbarIndex == INDEX_NONE)
	{
		for (int32 Index = 0; Index < HotbarSlotIndices.Num(); ++Index)
		{
			if (HotbarSlotIndices[Index] == INDEX_NONE)
			{
				HotbarIndex = Index;
				break;
			}
		}
		if (HotbarIndex == INDEX_NONE)
		{
			HotbarIndex = 0;
		}
		AssignToHotbar(InventorySlotIndex, HotbarIndex);
	}

	return ActivateHotbarSlot(HotbarIndex);
}

void UInventoryComponent::SetInventoryOpen(const bool bOpen)
{
	if (bInventoryOpen == bOpen)
	{
		return;
	}

	bInventoryOpen = bOpen;
	if (APawn* Pawn = GetOwnerPawn())
	{
		if (APlayerController* PC = Cast<APlayerController>(Pawn->GetController()))
		{
			if (bInventoryOpen)
			{
				PC->SetShowMouseCursor(true);
				FInputModeGameAndUI Mode;
				Mode.SetHideCursorDuringCapture(false);
				PC->SetInputMode(Mode);
			}
			else
			{
				PC->SetShowMouseCursor(false);
				FInputModeGameOnly Mode;
				PC->SetInputMode(Mode);
			}
		}
	}
	OnInventoryChanged.Broadcast();
}

void UInventoryComponent::ToggleInventory()
{
	SetInventoryOpen(!bInventoryOpen);
}

bool UInventoryComponent::DropItemToWorld(const int32 InventorySlotIndex)
{
	if (!Slots.IsValidIndex(InventorySlotIndex) || !Slots[InventorySlotIndex].ItemClass)
	{
		return false;
	}

	UWorld* World = GetWorld();
	APawn* Pawn = GetOwnerPawn();
	if (!World || !Pawn)
	{
		return false;
	}

	const TSubclassOf<AEquippableItem> ItemClass = Slots[InventorySlotIndex].ItemClass;
	if (!RemoveAt(InventorySlotIndex))
	{
		return false;
	}

	const FVector Forward = Pawn->GetActorForwardVector();
	const FVector SpawnLoc = Pawn->GetActorLocation() + Forward * 90.0f + FVector(0.0f, 0.0f, 30.0f);
	const FRotator SpawnRot = Pawn->GetActorRotation();

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	AEquippableItem* WorldItem = World->SpawnActor<AEquippableItem>(ItemClass, SpawnLoc, SpawnRot, Params);
	if (!WorldItem)
	{
		// Вернуть в инвентарь при сбое спавна.
		int32 Restored = INDEX_NONE;
		TryAddItem(ItemClass, Restored);
		return false;
	}

	WorldItem->ConfigureAsWorldPickup();
	WorldItem->OnDropped(Pawn);
	return true;
}

void UInventoryComponent::UnequipHeldInternal()
{
	if (AEquippableItem* Item = HeldItem.Get())
	{
		Item->OnUnequipped(GetOwnerPawn());
		Item->Destroy();
	}
	HeldItem = nullptr;
}

AEquippableItem* UInventoryComponent::SpawnHeldVisual(const TSubclassOf<AEquippableItem> ItemClass)
{
	UWorld* World = GetWorld();
	APawn* Pawn = GetOwnerPawn();
	if (!World || !Pawn || !ItemClass)
	{
		return nullptr;
	}

	FActorSpawnParameters Params;
	Params.Owner = Pawn;
	Params.Instigator = Pawn;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AEquippableItem* Item = World->SpawnActor<AEquippableItem>(ItemClass, Pawn->GetActorTransform(), Params);
	if (Item)
	{
		Item->ConfigureAsHeldVisual();
	}
	return Item;
}

APawn* UInventoryComponent::GetOwnerPawn() const
{
	return Cast<APawn>(GetOwner());
}
