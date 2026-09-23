#include "UsableEquipmentPromptWidget.h"

#include "Brushes/SlateRoundedBoxBrush.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/SOverlay.h"

class SUsableEquipmentCallout : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SUsableEquipmentCallout) {}
		SLATE_ATTRIBUTE(FText, Title)
		SLATE_ATTRIBUTE(FText, Action)
		SLATE_ATTRIBUTE(FVector2D, Anchor)
		SLATE_ATTRIBUTE(bool, ShowAction)
		SLATE_ATTRIBUTE(bool, OnScreen)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		Title = InArgs._Title;
		Action = InArgs._Action;
		Anchor = InArgs._Anchor;
		ShowAction = InArgs._ShowAction;
		OnScreen = InArgs._OnScreen;
	}

	virtual FVector2D ComputeDesiredSize(float) const override
	{
		return FVector2D(8.0f, 8.0f);
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
		if (!OnScreen.Get())
		{
			return LayerId;
		}

		FSlateFontInfo TitleFont = FCoreStyle::GetDefaultFontStyle("Bold", 11);
		FSlateFontInfo ActionFont = FCoreStyle::GetDefaultFontStyle("Regular", 11);
		const FSlateFontInfo KeyFont = FCoreStyle::GetDefaultFontStyle("Bold", 12);
		TitleFont.LetterSpacing = 140;
		ActionFont.LetterSpacing = 80;
		ActionFont.OutlineSettings = FFontOutlineSettings(1, FLinearColor(0.0f, 0.0f, 0.0f, 0.95f));
		const TSharedRef<FSlateFontMeasure> Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();

		const FText TitleText = Title.Get().ToUpper();
		const FText ActionText = Action.Get().ToUpper();
		const FText KeyText = FText::FromString(TEXT("E"));

		const FVector2D TitleSize = FVector2D(Measure->Measure(TitleText, TitleFont));
		const FVector2D ActionSize = FVector2D(Measure->Measure(ActionText, ActionFont));
		const FVector2D KeySize = FVector2D(Measure->Measure(KeyText, KeyFont));

		const float PlatePadX = 10.0f;
		const float PlatePadY = 3.0f;
		const FVector2D PlateSize(TitleSize.X + PlatePadX * 2.0f, FMath::Max(TitleSize.Y + PlatePadY * 2.0f, 22.0f));
		const float KeyBox = 22.0f;
		const bool bDrawAction = ShowAction.Get() && !ActionText.IsEmpty();
		const float ActionGap = 10.0f;
		const float KeyGap = 8.0f;
		float RowWidth = PlateSize.X + KeyGap + KeyBox;
		if (bDrawAction)
		{
			RowWidth += ActionGap + ActionSize.X;
		}

		const FVector2D View = AllottedGeometry.GetLocalSize();
		const FVector2D AnchorPos = Anchor.Get();
		const float LineLength = 108.0f;
		FVector2D PlatePos(
			AnchorPos.X - PlateSize.X * 0.5f,
			AnchorPos.Y - LineLength - PlateSize.Y);
		PlatePos.X = FMath::Clamp(PlatePos.X, 12.0f, FMath::Max(12.0f, View.X - RowWidth - 12.0f));
		PlatePos.Y = FMath::Clamp(PlatePos.Y, 12.0f, FMath::Max(12.0f, View.Y - PlateSize.Y - 12.0f));

		const float DotRadius = 4.0f;
		const FVector2D LineStart(PlatePos.X + PlateSize.X * 0.5f, PlatePos.Y + PlateSize.Y);
		const FVector2D LineEnd(AnchorPos.X, AnchorPos.Y - DotRadius);

		static const FSlateRoundedBoxBrush PlateBrush(FLinearColor::White, 0.0f, FVector2f(32.0f, 22.0f));
		static const FSlateRoundedBoxBrush KeyBrush(FLinearColor::White, 1.0f, FVector2f(22.0f, 22.0f));
		static const FSlateRoundedBoxBrush DotBrush(FLinearColor::White, 4.0f, FVector2f(8.0f, 8.0f));

		TArray<FVector2D> LinePoints;
		LinePoints.Add(LineStart);
		LinePoints.Add(LineEnd);
		FSlateDrawElement::MakeLines(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(),
			LinePoints,
			ESlateDrawEffect::None,
			FLinearColor::White,
			true,
			1.25f);

		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId + 1,
			AllottedGeometry.ToPaintGeometry(
				FVector2f(DotRadius * 2.0f, DotRadius * 2.0f),
				FSlateLayoutTransform(FVector2f(AnchorPos.X - DotRadius, AnchorPos.Y - DotRadius))),
			&DotBrush,
			ESlateDrawEffect::None,
			FLinearColor::White);

		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId + 2,
			AllottedGeometry.ToPaintGeometry(FVector2f(PlateSize), FSlateLayoutTransform(FVector2f(PlatePos))),
			&PlateBrush,
			ESlateDrawEffect::None,
			FLinearColor::White);

		const FVector2D TitlePos(
			PlatePos.X + PlatePadX,
			PlatePos.Y + (PlateSize.Y - TitleSize.Y) * 0.5f);
		FSlateDrawElement::MakeText(
			OutDrawElements,
			LayerId + 3,
			AllottedGeometry.ToPaintGeometry(FVector2f(TitleSize), FSlateLayoutTransform(FVector2f(TitlePos))),
			TitleText,
			TitleFont,
			ESlateDrawEffect::None,
			FLinearColor::Black);

		float CursorX = PlatePos.X + PlateSize.X + KeyGap;
		if (bDrawAction)
		{
			const FVector2D ActionPos(CursorX, PlatePos.Y + (PlateSize.Y - ActionSize.Y) * 0.5f);
			FSlateDrawElement::MakeText(
				OutDrawElements,
				LayerId + 3,
				AllottedGeometry.ToPaintGeometry(FVector2f(ActionSize), FSlateLayoutTransform(FVector2f(ActionPos))),
				ActionText,
				ActionFont,
				ESlateDrawEffect::None,
				FLinearColor::White);
			CursorX += ActionSize.X + ActionGap;
		}

		const FVector2D KeyPos(CursorX, PlatePos.Y + (PlateSize.Y - KeyBox) * 0.5f);
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId + 2,
			AllottedGeometry.ToPaintGeometry(FVector2f(KeyBox, KeyBox), FSlateLayoutTransform(FVector2f(KeyPos))),
			&KeyBrush,
			ESlateDrawEffect::None,
			FLinearColor::White);

		const FVector2D KeyTextPos(
			KeyPos.X + (KeyBox - KeySize.X) * 0.5f,
			KeyPos.Y + (KeyBox - KeySize.Y) * 0.5f);
		FSlateDrawElement::MakeText(
			OutDrawElements,
			LayerId + 3,
			AllottedGeometry.ToPaintGeometry(FVector2f(KeySize), FSlateLayoutTransform(FVector2f(KeyTextPos))),
			KeyText,
			KeyFont,
			ESlateDrawEffect::None,
			FLinearColor::Black);

		return LayerId + 4;
	}

private:
	TAttribute<FText> Title;
	TAttribute<FText> Action;
	TAttribute<FVector2D> Anchor;
	TAttribute<bool> ShowAction;
	TAttribute<bool> OnScreen;
};

void UUsableEquipmentPromptWidget::SetCallout(
	const FText& InTitle,
	const FText& InAction,
	const FVector2D InAnchorSlate,
	const bool bInOnScreen,
	const bool bInShowAction)
{
	Title = InTitle;
	Action = InAction;
	AnchorSlate = InAnchorSlate;
	bOnScreen = bInOnScreen;
	bShowAction = bInShowAction;
	if (Callout.IsValid())
	{
		Callout->Invalidate(EInvalidateWidget::Paint);
	}
}

TSharedRef<SWidget> UUsableEquipmentPromptWidget::RebuildWidget()
{
	return SNew(SOverlay)
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SAssignNew(Callout, SUsableEquipmentCallout)
			.Title_Lambda([this]() { return Title; })
			.Action_Lambda([this]() { return Action; })
			.Anchor_Lambda([this]() { return AnchorSlate; })
			.ShowAction_Lambda([this]() { return bShowAction; })
			.OnScreen_Lambda([this]() { return bOnScreen; })
		];
}
