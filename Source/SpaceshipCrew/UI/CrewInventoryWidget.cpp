#include "CrewInventoryWidget.h"

#include "Brushes/SlateRoundedBoxBrush.h"
#include "Crew/EquippableItem.h"
#include "Crew/InventoryComponent.h"
#include "CrewInventoryDragDrop.h"
#include "Engine/Texture2D.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateBrush.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/SOverlay.h"

namespace CrewInventoryPrivate
{
struct FInvSlotView
{
	int32 SlotIndex = INDEX_NONE;
	FText Label;
	TWeakObjectPtr<UTexture2D> Icon;
	bool bFilled = false;
};

constexpr int32 Columns = 4;
constexpr float Cell = 56.0f;
constexpr float Gap = 8.0f;
constexpr float PanelPad = 16.0f;
constexpr float TitleH = 36.0f;
constexpr float CloseSize = 22.0f;

FVector2D PanelSize(const int32 SlotCount)
{
	const int32 Rows = FMath::Max(1, FMath::DivideAndRoundUp(FMath::Max(SlotCount, 1), Columns));
	const float PanelW = Columns * Cell + (Columns - 1) * Gap + PanelPad * 2.0f;
	const float PanelH = TitleH + Rows * Cell + (Rows - 1) * Gap + PanelPad * 2.0f;
	return FVector2D(PanelW, PanelH);
}
} // namespace

class SCrewInventoryPanel : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SCrewInventoryPanel) {}
	SLATE_END_ARGS()

	DECLARE_DELEGATE_RetVal_TwoParams(FReply, FSlotClicked, int32, const FPointerEvent&);
	DECLARE_DELEGATE_RetVal(FReply, FCloseClicked);
	DECLARE_DELEGATE_RetVal_OneParam(bool, FSlotDropOp, TSharedPtr<FCrewInventoryDragDropOp>);

	void Construct(const FArguments&)
	{
	}

	void SetInventory(UInventoryComponent* InInventory)
	{
		Inventory = InInventory;
	}

	void SetSlots(TArray<CrewInventoryPrivate::FInvSlotView> InSlots, const bool bInVisible)
	{
		Slots = MoveTemp(InSlots);
		bVisible = bInVisible;
		IconBrushes.Reset();
		IconBrushes.SetNum(Slots.Num());
		for (int32 Index = 0; Index < Slots.Num(); ++Index)
		{
			if (UTexture2D* Icon = Slots[Index].Icon.Get())
			{
				IconBrushes[Index].DrawAs = ESlateBrushDrawType::Image;
				IconBrushes[Index].TintColor = FSlateColor(FLinearColor::White);
				IconBrushes[Index].SetResourceObject(Icon);
				IconBrushes[Index].ImageSize = FVector2f(40.0f, 40.0f);
			}
		}
		Invalidate(EInvalidateWidget::Paint | EInvalidateWidget::Layout);
	}

	FSlotClicked& OnSlotClicked() { return SlotClicked; }
	FCloseClicked& OnCloseClicked() { return CloseClicked; }
	FSlotDropOp& OnSlotDropOp() { return SlotDropOp; }

	virtual FVector2D ComputeDesiredSize(float) const override
	{
		if (!bVisible)
		{
			return FVector2D::ZeroVector;
		}
		return CrewInventoryPrivate::PanelSize(Slots.Num());
	}

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		if (!bVisible)
		{
			return FReply::Unhandled();
		}

		PendingSlotIndex = INDEX_NONE;
		bDragInitiated = false;

		if (IsOverClose(MyGeometry, MouseEvent.GetScreenSpacePosition())
			&& MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton
			&& CloseClicked.IsBound())
		{
			return CloseClicked.Execute();
		}

		const int32 Hit = HitTestSlot(MyGeometry, MouseEvent.GetScreenSpacePosition());
		if (Hit == INDEX_NONE)
		{
			return FReply::Unhandled();
		}

		if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton && SlotClicked.IsBound())
		{
			return SlotClicked.Execute(Hit, MouseEvent);
		}

		if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
		{
			PendingSlotIndex = Hit;
			const bool bFilled = IsSlotFilled(Hit);
			if (bFilled)
			{
				return FReply::Handled()
					.DetectDrag(AsShared(), EKeys::LeftMouseButton)
					.CaptureMouse(AsShared());
			}
			return FReply::Handled().CaptureMouse(AsShared());
		}

		return FReply::Unhandled();
	}

	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		FReply Reply = FReply::Unhandled();
		if (HasMouseCapture())
		{
			Reply = FReply::Handled().ReleaseMouseCapture();
		}

		if (!bVisible || MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
		{
			PendingSlotIndex = INDEX_NONE;
			return Reply;
		}

		if (!bDragInitiated && PendingSlotIndex != INDEX_NONE && SlotClicked.IsBound())
		{
			const int32 Slot = PendingSlotIndex;
			PendingSlotIndex = INDEX_NONE;
			return SlotClicked.Execute(Slot, MouseEvent);
		}

		PendingSlotIndex = INDEX_NONE;
		return Reply.IsEventHandled() ? Reply : FReply::Unhandled();
	}

	virtual FReply OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		(void)MyGeometry;
		(void)MouseEvent;
		if (PendingSlotIndex == INDEX_NONE || !IsSlotFilled(PendingSlotIndex))
		{
			return FReply::Unhandled();
		}

		bDragInitiated = true;
		FText Label;
		for (const CrewInventoryPrivate::FInvSlotView& Slot : Slots)
		{
			if (Slot.SlotIndex == PendingSlotIndex)
			{
				Label = Slot.Label;
				break;
			}
		}
		return FReply::Handled().BeginDragDrop(
			FCrewInventoryDragDropOp::NewFromInventory(Inventory.Get(), PendingSlotIndex, Label));
	}

	virtual FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override
	{
		const TSharedPtr<FCrewInventoryDragDropOp> Op = DragDropEvent.GetOperationAs<FCrewInventoryDragDropOp>();
		if (!Op.IsValid())
		{
			return FReply::Unhandled();
		}

		const int32 Target = HitTestSlot(MyGeometry, DragDropEvent.GetScreenSpacePosition());
		if (Target == INDEX_NONE)
		{
			// Не на слот — пусть OnDrop операции выкинет в мир / снимет с хотбара.
			return FReply::Unhandled();
		}

		if (Op->IsFromHotbar())
		{
			UInventoryComponent* Inv = Inventory.Get();
			return (Inv && Inv->ClearHotbarSlot(Op->HotbarIndex))
				? FReply::Handled()
				: FReply::Unhandled();
		}

		UInventoryComponent* Inv = Inventory.Get();
		return (Inv && Inv->SwapSlots(Op->InventorySlotIndex, Target))
			? FReply::Handled()
			: FReply::Unhandled();
	}

	virtual FReply OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override
	{
		const TSharedPtr<FCrewInventoryDragDropOp> Op = DragDropEvent.GetOperationAs<FCrewInventoryDragDropOp>();
		if (!Op.IsValid())
		{
			return FReply::Unhandled();
		}
		return HitTestSlot(MyGeometry, DragDropEvent.GetScreenSpacePosition()) != INDEX_NONE
			? FReply::Handled()
			: FReply::Unhandled();
	}

	virtual int32 OnPaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override
	{
		(void)Args;
		(void)MyCullingRect;
		(void)InWidgetStyle;
		(void)bParentEnabled;

		if (!bVisible)
		{
			return LayerId;
		}

		using namespace CrewInventoryPrivate;
		const FVector2D Size = PanelSize(Slots.Num());
		const FVector2D PanelPos = FVector2D::ZeroVector;

		static const FSlateRoundedBoxBrush RoundBrush(FLinearColor::White, 4.0f, FVector2f(64.0f, 64.0f));

		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(FVector2f(Size), FSlateLayoutTransform(FVector2f(PanelPos))),
			&RoundBrush,
			ESlateDrawEffect::None,
			FLinearColor(0.04f, 0.05f, 0.07f, 0.94f));

		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(
				FVector2f(Size.X, 2.0f),
				FSlateLayoutTransform(FVector2f(PanelPos))),
			&RoundBrush,
			ESlateDrawEffect::None,
			FLinearColor(0.55f, 0.72f, 0.95f, 0.85f));

		FSlateFontInfo TitleFont = FCoreStyle::GetDefaultFontStyle("Bold", 13);
		FSlateFontInfo NameFont = FCoreStyle::GetDefaultFontStyle("Bold", 10);
		FSlateFontInfo CloseFont = FCoreStyle::GetDefaultFontStyle("Bold", 14);
		TitleFont.OutlineSettings = FFontOutlineSettings(1, FLinearColor(0.0f, 0.0f, 0.0f, 0.9f));
		NameFont.OutlineSettings = FFontOutlineSettings(1, FLinearColor(0.0f, 0.0f, 0.0f, 0.95f));
		const TSharedRef<FSlateFontMeasure> Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();

		const FText Title = NSLOCTEXT("SpaceshipCrew", "Inventory_Title", "ИНВЕНТАРЬ");
		const FVector2D TitleSize = FVector2D(Measure->Measure(Title, TitleFont));
		FSlateDrawElement::MakeText(
			OutDrawElements,
			LayerId + 1,
			AllottedGeometry.ToPaintGeometry(
				FVector2f(TitleSize),
				FSlateLayoutTransform(FVector2f(PanelPos.X + PanelPad, PanelPos.Y + 12.0f))),
			Title,
			TitleFont,
			ESlateDrawEffect::None,
			FLinearColor(0.92f, 0.95f, 1.0f, 1.0f));

		// Крестик закрытия.
		const FVector2D ClosePos(Size.X - PanelPad - CloseSize, PanelPad * 0.5f);
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId + 1,
			AllottedGeometry.ToPaintGeometry(
				FVector2f(CloseSize, CloseSize),
				FSlateLayoutTransform(FVector2f(ClosePos))),
			&RoundBrush,
			ESlateDrawEffect::None,
			FLinearColor(0.45f, 0.18f, 0.18f, 0.95f));
		const FText CloseText = FText::FromString(TEXT("×"));
		const FVector2D CloseTextSize = FVector2D(Measure->Measure(CloseText, CloseFont));
		FSlateDrawElement::MakeText(
			OutDrawElements,
			LayerId + 2,
			AllottedGeometry.ToPaintGeometry(
				FVector2f(CloseTextSize),
				FSlateLayoutTransform(FVector2f(
					ClosePos.X + (CloseSize - CloseTextSize.X) * 0.5f,
					ClosePos.Y + (CloseSize - CloseTextSize.Y) * 0.5f - 1.0f))),
			CloseText,
			CloseFont,
			ESlateDrawEffect::None,
			FLinearColor::White);

		const float GridOriginX = PanelPos.X + PanelPad;
		const float GridOriginY = PanelPos.Y + PanelPad + TitleH;

		for (int32 Index = 0; Index < Slots.Num(); ++Index)
		{
			const int32 Col = Index % Columns;
			const int32 Row = Index / Columns;
			const FVector2D Pos(GridOriginX + Col * (Cell + Gap), GridOriginY + Row * (Cell + Gap));
			const CrewInventoryPrivate::FInvSlotView& Slot = Slots[Index];

			const FLinearColor Border = Slot.bFilled
				? FLinearColor(0.75f, 0.85f, 1.0f, 0.95f)
				: FLinearColor(0.45f, 0.50f, 0.58f, 0.9f);
			const FLinearColor Fill = Slot.bFilled
				? FLinearColor(0.12f, 0.16f, 0.22f, 0.98f)
				: FLinearColor(0.07f, 0.08f, 0.10f, 0.96f);

			FSlateDrawElement::MakeBox(
				OutDrawElements,
				LayerId + 1,
				AllottedGeometry.ToPaintGeometry(FVector2f(Cell, Cell), FSlateLayoutTransform(FVector2f(Pos))),
				&RoundBrush,
				ESlateDrawEffect::None,
				Border);

			FSlateDrawElement::MakeBox(
				OutDrawElements,
				LayerId + 2,
				AllottedGeometry.ToPaintGeometry(
					FVector2f(Cell - 4.0f, Cell - 4.0f),
					FSlateLayoutTransform(FVector2f(Pos.X + 2.0f, Pos.Y + 2.0f))),
				&RoundBrush,
				ESlateDrawEffect::None,
				Fill);

			if (!Slot.bFilled)
			{
				continue;
			}

			if (IconBrushes.IsValidIndex(Index) && IconBrushes[Index].GetResourceObject())
			{
				FSlateDrawElement::MakeBox(
					OutDrawElements,
					LayerId + 3,
					AllottedGeometry.ToPaintGeometry(
						FVector2f(36.0f, 36.0f),
						FSlateLayoutTransform(FVector2f(Pos.X + 10.0f, Pos.Y + 6.0f))),
					&IconBrushes[Index],
					ESlateDrawEffect::None,
					FLinearColor::White);
			}

			const FText Label = Slot.Label.IsEmpty()
				? NSLOCTEXT("SpaceshipCrew", "Inventory_Item", "ПРЕДМЕТ")
				: FText::FromString(Slot.Label.ToString().Left(10));
			const FVector2D NameSize = FVector2D(Measure->Measure(Label, NameFont));
			const float TextY = IconBrushes.IsValidIndex(Index) && IconBrushes[Index].GetResourceObject()
				? Pos.Y + Cell - NameSize.Y - 4.0f
				: Pos.Y + (Cell - NameSize.Y) * 0.5f;
			FSlateDrawElement::MakeText(
				OutDrawElements,
				LayerId + 4,
				AllottedGeometry.ToPaintGeometry(
					FVector2f(NameSize),
					FSlateLayoutTransform(FVector2f(Pos.X + (Cell - NameSize.X) * 0.5f, TextY))),
				Label,
				NameFont,
				ESlateDrawEffect::None,
				FLinearColor(1.0f, 0.92f, 0.45f, 1.0f));
		}

		return LayerId + 5;
	}

private:
	bool IsSlotFilled(const int32 SlotIndex) const
	{
		for (const CrewInventoryPrivate::FInvSlotView& Slot : Slots)
		{
			if (Slot.SlotIndex == SlotIndex)
			{
				return Slot.bFilled;
			}
		}
		return false;
	}

	bool IsOverClose(const FGeometry& MyGeometry, const FVector2D ScreenPos) const
	{
		using namespace CrewInventoryPrivate;
		const FVector2D Local = MyGeometry.AbsoluteToLocal(ScreenPos);
		const FVector2D Size = PanelSize(Slots.Num());
		const FVector2D ClosePos(Size.X - PanelPad - CloseSize, PanelPad * 0.5f);
		const FSlateRect Rect(ClosePos.X, ClosePos.Y, ClosePos.X + CloseSize, ClosePos.Y + CloseSize);
		return Rect.ContainsPoint(Local);
	}

	int32 HitTestSlot(const FGeometry& MyGeometry, const FVector2D ScreenPos) const
	{
		if (!bVisible)
		{
			return INDEX_NONE;
		}

		using namespace CrewInventoryPrivate;
		const FVector2D Local = MyGeometry.AbsoluteToLocal(ScreenPos);
		const float GridOriginX = PanelPad;
		const float GridOriginY = PanelPad + TitleH;

		for (int32 Index = 0; Index < Slots.Num(); ++Index)
		{
			const int32 Col = Index % Columns;
			const int32 Row = Index / Columns;
			const FVector2D Pos(GridOriginX + Col * (Cell + Gap), GridOriginY + Row * (Cell + Gap));
			const FSlateRect Rect(Pos.X, Pos.Y, Pos.X + Cell, Pos.Y + Cell);
			if (Rect.ContainsPoint(Local))
			{
				return Slots[Index].SlotIndex;
			}
		}
		return INDEX_NONE;
	}

	TArray<CrewInventoryPrivate::FInvSlotView> Slots;
	TArray<FSlateBrush> IconBrushes;
	TWeakObjectPtr<UInventoryComponent> Inventory;
	bool bVisible = false;
	int32 PendingSlotIndex = INDEX_NONE;
	bool bDragInitiated = false;
	FSlotClicked SlotClicked;
	FCloseClicked CloseClicked;
	FSlotDropOp SlotDropOp;
};

void UCrewInventoryWidget::BindInventory(UInventoryComponent* InInventory)
{
	if (Inventory.Get() == InInventory)
	{
		RefreshFromInventory();
		return;
	}

	if (UInventoryComponent* Old = Inventory.Get())
	{
		Old->OnInventoryChanged.RemoveAll(this);
	}

	Inventory = InInventory;
	if (UInventoryComponent* NewInv = Inventory.Get())
	{
		NewInv->OnInventoryChanged.AddDynamic(this, &UCrewInventoryWidget::HandleInventoryChanged);
	}
	RefreshFromInventory();
}

void UCrewInventoryWidget::HandleInventoryChanged()
{
	RefreshFromInventory();
}

void UCrewInventoryWidget::CloseInventory()
{
	if (UInventoryComponent* Inv = Inventory.Get())
	{
		Inv->SetInventoryOpen(false);
	}
}

void UCrewInventoryWidget::RefreshFromInventory()
{
	TArray<CrewInventoryPrivate::FInvSlotView> Views;
	bool bOpen = false;
	if (UInventoryComponent* Inv = Inventory.Get())
	{
		bOpen = Inv->IsInventoryOpen();
		const TArray<FInventoryEntry>& InvSlots = Inv->GetSlots();
		Views.SetNum(InvSlots.Num());
		for (int32 Index = 0; Index < InvSlots.Num(); ++Index)
		{
			CrewInventoryPrivate::FInvSlotView& View = Views[Index];
			View.SlotIndex = Index;
			if (InvSlots[Index].ItemClass)
			{
				View.bFilled = true;
				if (const AEquippableItem* CDO = InvSlots[Index].ItemClass.GetDefaultObject())
				{
					View.Label = CDO->GetItemDisplayName();
					View.Icon = CDO->GetInventoryIcon();
				}
				if (View.Label.IsEmpty())
				{
					View.Label = FText::FromString(InvSlots[Index].ItemClass->GetName());
				}
			}
		}
	}

	SetVisibility(bOpen ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	if (Panel.IsValid())
	{
		Panel->SetInventory(Inventory.Get());
		Panel->SetSlots(MoveTemp(Views), bOpen);
		Panel->SetVisibility(bOpen ? EVisibility::Visible : EVisibility::Collapsed);
	}
}

FReply UCrewInventoryWidget::OnCloseClicked()
{
	CloseInventory();
	return FReply::Handled();
}

FReply UCrewInventoryWidget::OnSlotClicked(const int32 SlotIndex, const FPointerEvent& MouseEvent)
{
	UInventoryComponent* Inv = Inventory.Get();
	if (!Inv || SlotIndex == INDEX_NONE)
	{
		return FReply::Unhandled();
	}

	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		Inv->EquipFromInventoryIndex(SlotIndex);
		return FReply::Handled();
	}

	if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		int32 TargetHotbar = Inv->GetActiveHotbarIndex();
		if (TargetHotbar == INDEX_NONE)
		{
			const TArray<int32>& Hotbar = Inv->GetHotbarSlotIndices();
			TargetHotbar = 0;
			for (int32 Index = 0; Index < Hotbar.Num(); ++Index)
			{
				if (Hotbar[Index] == INDEX_NONE)
				{
					TargetHotbar = Index;
					break;
				}
			}
		}
		Inv->AssignToHotbar(SlotIndex, TargetHotbar);
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

void UCrewInventoryWidget::NativeDestruct()
{
	if (UInventoryComponent* Inv = Inventory.Get())
	{
		Inv->OnInventoryChanged.RemoveAll(this);
	}
	Inventory = nullptr;
	Super::NativeDestruct();
}

TSharedRef<SWidget> UCrewInventoryWidget::RebuildWidget()
{
	TSharedRef<SCrewInventoryPanel> NewPanel = SAssignNew(Panel, SCrewInventoryPanel);
	NewPanel->OnSlotClicked().BindUObject(this, &UCrewInventoryWidget::OnSlotClicked);
	NewPanel->OnCloseClicked().BindUObject(this, &UCrewInventoryWidget::OnCloseClicked);
	NewPanel->SetInventory(Inventory.Get());
	SetVisibility(ESlateVisibility::Collapsed);
	RefreshFromInventory();
	return SNew(SOverlay)
		+ SOverlay::Slot()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Center)
		.Padding(FMargin(0.0f, 0.0f, 24.0f, 0.0f))
		[
			NewPanel
		];
}
