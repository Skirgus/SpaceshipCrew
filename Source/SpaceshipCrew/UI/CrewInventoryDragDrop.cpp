#include "CrewInventoryDragDrop.h"

#include "Crew/InventoryComponent.h"

void FCrewInventoryDragDropOp::OnDrop(const bool bDropWasHandled, const FPointerEvent& MouseEvent)
{
	(void)MouseEvent;
	if (bDropWasHandled)
	{
		return;
	}

	UInventoryComponent* Inv = Inventory.Get();
	if (!Inv)
	{
		return;
	}

	if (IsFromInventory())
	{
		// Бросок на свободное место экрана — выкинуть в мир.
		Inv->DropItemToWorld(InventorySlotIndex);
	}
	else if (IsFromHotbar())
	{
		// Утащили с хотбара в пустоту — снять с панели быстрого доступа.
		Inv->ClearHotbarSlot(HotbarIndex);
	}
}
