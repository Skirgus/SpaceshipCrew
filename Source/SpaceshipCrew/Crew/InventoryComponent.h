#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventoryComponent.generated.h"

class AEquippableItem;
class APawn;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHotbarChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHeldItemChanged);

USTRUCT(BlueprintType)
struct FInventoryEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TSubclassOf<AEquippableItem> ItemClass;

	// Задел: InstanceId, Durability, Ammo, StackCount
};

/**
 * Инвентарь экипажа: динамическая ёмкость, хотбар, слот в руках.
 */
UCLASS(ClassGroup = (Crew), meta = (BlueprintSpawnableComponent))
class SPACESHIPCREW_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetCapacity() const { return Capacity; }

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetCapacity(int32 NewCapacity);

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetHotbarSize() const { return HotbarSize; }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	const TArray<FInventoryEntry>& GetSlots() const { return Slots; }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	const TArray<int32>& GetHotbarSlotIndices() const { return HotbarSlotIndices; }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetActiveHotbarIndex() const { return ActiveHotbarIndex; }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	AEquippableItem* GetHeldItem() const { return HeldItem.Get(); }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool IsInventoryOpen() const { return bInventoryOpen; }

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool TryAddItem(TSubclassOf<AEquippableItem> ItemClass, int32& OutSlotIndex);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveAt(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AssignToHotbar(int32 InventorySlotIndex, int32 HotbarIndex);

	/** Убрать привязку слота хотбара (предмет остаётся в инвентаре). */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool ClearHotbarSlot(int32 HotbarIndex);

	/** Поменять местами два слота инвентаря (и переназначить индексы хотбара). */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool SwapSlots(int32 SlotA, int32 SlotB);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool ActivateHotbarSlot(int32 HotbarIndex);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void ClearActiveSlot();

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool EquipFromInventoryIndex(int32 InventorySlotIndex);

	/** Выкинуть предмет из инвентаря в мир перед персонажем (можно подобрать снова). */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool DropItemToWorld(int32 InventorySlotIndex);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetInventoryOpen(bool bOpen);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void ToggleInventory();

	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryChanged OnInventoryChanged;

	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnHotbarChanged OnHotbarChanged;

	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnHeldItemChanged OnHeldItemChanged;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory", meta = (ClampMin = "1"))
	int32 Capacity = 12;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory", meta = (ClampMin = "1", ClampMax = "10"))
	int32 HotbarSize = 5;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	TArray<FInventoryEntry> Slots;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	TArray<int32> HotbarSlotIndices;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	int32 ActiveHotbarIndex = INDEX_NONE;

	UPROPERTY(Transient)
	TWeakObjectPtr<AEquippableItem> HeldItem;

	UPROPERTY(Transient)
	bool bInventoryOpen = false;

	void EnsureHotbarSize();
	void UnequipHeldInternal();
	AEquippableItem* SpawnHeldVisual(TSubclassOf<AEquippableItem> ItemClass);
	APawn* GetOwnerPawn() const;
};
