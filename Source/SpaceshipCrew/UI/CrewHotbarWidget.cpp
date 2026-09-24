#include "CrewHotbarWidget.h"

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

namespace CrewHotbarPrivate
{
struct FHotbarSlotView
{
	int32 InventorySlotIndex = INDEX_NONE;
	FText Label;
	TWeakObjectPtr<UTexture2D> Icon;
	bool bActive = false;
	bool bFilled = false;
};

constexpr float Cell = 56.0f;
constexpr float Gap = 8.0f;
} // namespace

class SCrewHotbarPanel : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SCrewHotbarPanel) {}
	SLATE_END_ARGS()

	DECLARE_DELEGATE_RetVal_TwoParams(bool, FHotbarDropOp, TSharedPtr<FCrewInventoryDragDropOp>, int32 /*TargetHotbar*/);

	void Construct(const FArguments&)
	{
	}

	void SetInventory(UInventoryComponent* InInventory)
	{
		Inventory = InInventory;
	}

	void SetSlots(TArray<CrewHotbarPrivate::FHotbarSlotView> InSlots)
	{
		Slots = MoveTemp(InSlots);
		IconBrushes.Reset();
		IconBrushes.SetNum(Slots.Num());
		for (int32 Index = 0; Index < Slots.Num(); ++Index)
		{
			if (UTexture2D* Icon = Slots[Index].Icon.Get())
			{
				IconBrushes[Index].DrawAs = ESlateBrushDrawType::Image;
				IconBrushes[Index].TintColor = FSlateColor(FLinearColor::White);
				IconBrushes[Index].SetResourceObject(Icon);
				IconBrushes[Index].ImageSize = FVector2f(36.0f, 36.0f);
			}
		}
		Invalidate(EInvalidateWidget::Paint | EInvalidateWidget::Layout);
	}

	FHotbarDropOp& OnHotbarDropOp() { return HotbarDropOp; }

	virtual FVector2D ComputeDesiredSize(float) const override
	{
		using namespace CrewHotbarPrivate;
		const int32 Count = FMath::Max(1, Slots.Num());
		return FVector2D(Count * Cell + (Count - 1) * Gap, Cell);
	}

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		PendingHotbarIndex = INDEX_NONE;
		bDragInitiated = false;

		if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
		{
			return FReply::Unhandled();
		}

		const int32 Hit = HitTestSlot(MyGeometry, MouseEvent.GetScreenSpacePosition());
		if (Hit == INDEX_NONE || !Slots.IsValidIndex(Hit) || !Slots[Hit].bFilled)
		{
			return FReply::Unhandled();
		}

		PendingHotbarIndex = Hit;
		return FReply::Handled()
			.DetectDrag(AsShared(), EKeys::LeftMouseButton)
			.CaptureMouse(AsShared());
	}

	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		(void)MyGeometry;
		FReply Reply = FReply::Unhandled();
		if (HasMouseCapture())
		{
			Reply = FReply::Handled().ReleaseMouseCapture();
		}
		PendingHotbarIndex = INDEX_NONE;
		bDragInitiated = false;
		return Reply;
	}

	virtual FReply OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		(void)MyGeometry;
		(void)MouseEvent;
		if (!Slots.IsValidIndex(PendingHotbarIndex) || !Slots[PendingHotbarIndex].bFilled)
		{
			return FReply::Unhandled();
		}

		bDragInitiated = true;
		const CrewHotbarPrivate::FHotbarSlotView& Slot = Slots[PendingHotbarIndex];
		return FReply::Handled().BeginDragDrop(
			FCrewInventoryDragDropOp::NewFromHotbar(
				Inventory.Get(),
				PendingHotbarIndex,
				Slot.InventorySlotIndex,
				Slot.Label));
	}

	virtual FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override
	{
		const TSharedPtr<FCrewInventoryDragDropOp> Op = DragDropEvent.GetOperationAs<FCrewInventoryDragDropOp>();
		if (!Op.IsValid() || !HotbarDropOp.IsBound())
		{
			return FReply::Unhandled();
		}
		const int32 HotbarIndex = HitTestSlot(MyGeometry, DragDropEvent.GetScreenSpacePosition());
		if (HotbarIndex == INDEX_NONE)
		{
			return FReply::Unhandled();
		}
		return HotbarDropOp.Execute(Op, HotbarIndex) ? FReply::Handled() : FReply::Unhandled();
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

		using namespace CrewHotbarPrivate;
		const int32 Count = Slots.Num();
		if (Count <= 0)
		{
			return LayerId;
		}

		static const FSlateRoundedBoxBrush RoundBrush(FLinearColor::White, 4.0f, FVector2f(64.0f, 64.0f));

		FSlateFontInfo KeyFont = FCoreStyle::GetDefaultFontStyle("Bold", 11);
		FSlateFontInfo NameFont = FCoreStyle::GetDefaultFontStyle("Bold", 9);
		KeyFont.OutlineSettings = FFontOutlineSettings(1, FLinearColor(0.0f, 0.0f, 0.0f, 0.95f));
		NameFont.OutlineSettings = FFontOutlineSettings(1, FLinearColor(0.0f, 0.0f, 0.0f, 0.95f));
		const TSharedRef<FSlateFontMeasure> Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();

		for (int32 Index = 0; Index < Count; ++Index)
		{
			const CrewHotbarPrivate::FHotbarSlotView& Slot = Slots[Index];
			const FVector2D Pos(Index * (Cell + Gap), 0.0f);

			const FLinearColor Border = Slot.bActive
				? FLinearColor(1.0f, 0.82f, 0.22f, 1.0f)
				: (Slot.bFilled
					? FLinearColor(0.70f, 0.82f, 1.0f, 0.95f)
					: FLinearColor(0.40f, 0.45f, 0.52f, 0.95f));
			const FLinearColor Fill = Slot.bActive
				? FLinearColor(0.18f, 0.15f, 0.06f, 0.96f)
				: FLinearColor(0.06f, 0.07f, 0.09f, 0.94f);

			FSlateDrawElement::MakeBox(
				OutDrawElements,
				LayerId,
				AllottedGeometry.ToPaintGeometry(FVector2f(Cell, Cell), FSlateLayoutTransform(FVector2f(Pos))),
				&RoundBrush,
				ESlateDrawEffect::None,
				Border);

			FSlateDrawElement::MakeBox(
				OutDrawElements,
				LayerId + 1,
				AllottedGeometry.ToPaintGeometry(
					FVector2f(Cell - 4.0f, Cell - 4.0f),
					FSlateLayoutTransform(FVector2f(Pos.X + 2.0f, Pos.Y + 2.0f))),
				&RoundBrush,
				ESlateDrawEffect::None,
				Fill);

			if (IconBrushes.IsValidIndex(Index) && IconBrushes[Index].GetResourceObject())
			{
				FSlateDrawElement::MakeBox(
					OutDrawElements,
					LayerId + 2,
					AllottedGeometry.ToPaintGeometry(
						FVector2f(32.0f, 32.0f),
						FSlateLayoutTransform(FVector2f(Pos.X + 12.0f, Pos.Y + 8.0f))),
					&IconBrushes[Index],
					ESlateDrawEffect::None,
					FLinearColor::White);
			}
			else if (Slot.bFilled && !Slot.Label.IsEmpty())
			{
				const FText Short = FText::FromString(Slot.Label.ToString().Left(8));
				const FVector2D NameSize = FVector2D(Measure->Measure(Short, NameFont));
				FSlateDrawElement::MakeText(
					OutDrawElements,
					LayerId + 3,
					AllottedGeometry.ToPaintGeometry(
						FVector2f(NameSize),
						FSlateLayoutTransform(FVector2f(
							Pos.X + (Cell - NameSize.X) * 0.5f,
							Pos.Y + (Cell - NameSize.Y) * 0.5f))),
					Short,
					NameFont,
					ESlateDrawEffect::None,
					FLinearColor(1.0f, 0.92f, 0.45f, 1.0f));
			}

			const FText KeyText = FText::AsNumber(Index + 1);
			const FVector2D KeySize = FVector2D(Measure->Measure(KeyText, KeyFont));
			FSlateDrawElement::MakeText(
				OutDrawElements,
				LayerId + 4,
				AllottedGeometry.ToPaintGeometry(
					FVector2f(KeySize),
					FSlateLayoutTransform(FVector2f(Pos.X + 5.0f, Pos.Y + 3.0f))),
				KeyText,
				KeyFont,
				ESlateDrawEffect::None,
				FLinearColor(0.95f, 0.97f, 1.0f, 1.0f));
		}

		return LayerId + 5;
	}

private:
	int32 HitTestSlot(const FGeometry& MyGeometry, const FVector2D ScreenPos) const
	{
		using namespace CrewHotbarPrivate;
		const FVector2D Local = MyGeometry.AbsoluteToLocal(ScreenPos);
		for (int32 Index = 0; Index < Slots.Num(); ++Index)
		{
			const FVector2D Pos(Index * (Cell + Gap), 0.0f);
			const FSlateRect Rect(Pos.X, Pos.Y, Pos.X + Cell, Pos.Y + Cell);
			if (Rect.ContainsPoint(Local))
			{
				return Index;
			}
		}
		return INDEX_NONE;
	}

	TArray<CrewHotbarPrivate::FHotbarSlotView> Slots;
	TArray<FSlateBrush> IconBrushes;
	TWeakObjectPtr<UInventoryComponent> Inventory;
	int32 PendingHotbarIndex = INDEX_NONE;
	bool bDragInitiated = false;
	FHotbarDropOp HotbarDropOp;
};

void UCrewHotbarWidget::BindInventory(UInventoryComponent* InInventory)
{
	if (Inventory.Get() == InInventory)
	{
		RefreshFromInventory();
		return;
	}

	if (UInventoryComponent* Old = Inventory.Get())
	{
		Old->OnInventoryChanged.RemoveAll(this);
		Old->OnHotbarChanged.RemoveAll(this);
	}

	Inventory = InInventory;
	if (UInventoryComponent* NewInv = Inventory.Get())
	{
		NewInv->OnInventoryChanged.AddDynamic(this, &UCrewHotbarWidget::HandleInventoryChanged);
		NewInv->OnHotbarChanged.AddDynamic(this, &UCrewHotbarWidget::HandleHotbarChanged);
	}
	RefreshFromInventory();
}

void UCrewHotbarWidget::HandleInventoryChanged()
{
	RefreshFromInventory();
}

void UCrewHotbarWidget::HandleHotbarChanged()
{
	RefreshFromInventory();
}

void UCrewHotbarWidget::RefreshFromInventory()
{
	TArray<CrewHotbarPrivate::FHotbarSlotView> Views;
	if (UInventoryComponent* Inv = Inventory.Get())
	{
		const TArray<int32>& Hotbar = Inv->GetHotbarSlotIndices();
		const TArray<FInventoryEntry>& InvSlots = Inv->GetSlots();
		const int32 Active = Inv->GetActiveHotbarIndex();
		Views.SetNum(Hotbar.Num());
		for (int32 Index = 0; Index < Hotbar.Num(); ++Index)
		{
			CrewHotbarPrivate::FHotbarSlotView& View = Views[Index];
			View.bActive = (Index == Active);
			const int32 InvIndex = Hotbar[Index];
			View.InventorySlotIndex = InvIndex;
			if (InvSlots.IsValidIndex(InvIndex) && InvSlots[InvIndex].ItemClass)
			{
				View.bFilled = true;
				if (const AEquippableItem* CDO = InvSlots[InvIndex].ItemClass.GetDefaultObject())
				{
					View.Label = CDO->GetItemDisplayName();
					View.Icon = CDO->GetInventoryIcon();
				}
				if (View.Label.IsEmpty())
				{
					View.Label = FText::FromString(InvSlots[InvIndex].ItemClass->GetName());
				}
			}
		}
	}

	if (Panel.IsValid())
	{
		Panel->SetInventory(Inventory.Get());
		Panel->SetSlots(MoveTemp(Views));
	}
}

bool UCrewHotbarWidget::HandleHotbarDrop(TSharedPtr<FCrewInventoryDragDropOp> Op, const int32 TargetHotbarIndex)
{
	UInventoryComponent* Inv = Inventory.Get();
	if (!Inv || !Op.IsValid())
	{
		return false;
	}

	if (Op->IsFromHotbar())
	{
		if (Op->HotbarIndex == TargetHotbarIndex)
		{
			return true;
		}
		const int32 InvSlot = Op->InventorySlotIndex;
		Inv->ClearHotbarSlot(Op->HotbarIndex);
		return Inv->AssignToHotbar(InvSlot, TargetHotbarIndex);
	}

	return Inv->AssignToHotbar(Op->InventorySlotIndex, TargetHotbarIndex);
}

void UCrewHotbarWidget::NativeDestruct()
{
	if (UInventoryComponent* Inv = Inventory.Get())
	{
		Inv->OnInventoryChanged.RemoveAll(this);
		Inv->OnHotbarChanged.RemoveAll(this);
	}
	Inventory = nullptr;
	Super::NativeDestruct();
}

TSharedRef<SWidget> UCrewHotbarWidget::RebuildWidget()
{
	TSharedRef<SCrewHotbarPanel> NewPanel = SAssignNew(Panel, SCrewHotbarPanel);
	NewPanel->OnHotbarDropOp().BindUObject(this, &UCrewHotbarWidget::HandleHotbarDrop);
	NewPanel->SetInventory(Inventory.Get());
	RefreshFromInventory();
	return SNew(SOverlay)
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Bottom)
		.Padding(FMargin(0.0f, 0.0f, 0.0f, 22.0f))
		[
			NewPanel
		];
}
