#pragma once

#include "CoreMinimal.h"
#include "Input/DragAndDrop.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"

class UInventoryComponent;

enum class ECrewInvDragSource : uint8
{
	Inventory,
	Hotbar
};

/** Drag из инвентаря или с хотбара. */
class FCrewInventoryDragDropOp : public FDragDropOperation
{
public:
	DRAG_DROP_OPERATOR_TYPE(FCrewInventoryDragDropOp, FDragDropOperation)

	static TSharedRef<FCrewInventoryDragDropOp> NewFromInventory(
		UInventoryComponent* InInventory,
		const int32 InInventorySlotIndex,
		const FText& InLabel)
	{
		TSharedRef<FCrewInventoryDragDropOp> Op = MakeShareable(new FCrewInventoryDragDropOp());
		Op->Source = ECrewInvDragSource::Inventory;
		Op->Inventory = InInventory;
		Op->InventorySlotIndex = InInventorySlotIndex;
		Op->HotbarIndex = INDEX_NONE;
		Op->Label = InLabel;
		Op->Construct();
		return Op;
	}

	static TSharedRef<FCrewInventoryDragDropOp> NewFromHotbar(
		UInventoryComponent* InInventory,
		const int32 InHotbarIndex,
		const int32 InInventorySlotIndex,
		const FText& InLabel)
	{
		TSharedRef<FCrewInventoryDragDropOp> Op = MakeShareable(new FCrewInventoryDragDropOp());
		Op->Source = ECrewInvDragSource::Hotbar;
		Op->Inventory = InInventory;
		Op->HotbarIndex = InHotbarIndex;
		Op->InventorySlotIndex = InInventorySlotIndex;
		Op->Label = InLabel;
		Op->Construct();
		return Op;
	}

	ECrewInvDragSource Source = ECrewInvDragSource::Inventory;
	TWeakObjectPtr<UInventoryComponent> Inventory;
	int32 InventorySlotIndex = INDEX_NONE;
	int32 HotbarIndex = INDEX_NONE;
	FText Label;

	bool IsFromInventory() const { return Source == ECrewInvDragSource::Inventory; }
	bool IsFromHotbar() const { return Source == ECrewInvDragSource::Hotbar; }

	virtual void OnDrop(bool bDropWasHandled, const FPointerEvent& MouseEvent) override;

	virtual TSharedPtr<SWidget> GetDefaultDecorator() const override
	{
		return SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(FLinearColor(0.10f, 0.12f, 0.16f, 0.92f))
			.Padding(FMargin(10.0f, 6.0f))
			[
				SNew(STextBlock)
				.Text(Label.IsEmpty()
					? NSLOCTEXT("SpaceshipCrew", "Inventory_Drag", "…")
					: Label)
				.ColorAndOpacity(FLinearColor(1.0f, 0.92f, 0.45f, 1.0f))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
			];
	}
};
