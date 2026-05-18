#include "SSpaceshipMainMenuBase.h"

#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Menu/SpaceshipCrewLevelTravel.h"
#include "SSpaceshipBlueprintPicker.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SpaceshipMainMenuBase"

namespace SpaceshipCrewSlateMenu
{
	static constexpr float RootLeftPadding = 72.0f;
	static constexpr float RootVerticalPadding = 56.0f;
	static constexpr float MenuColumnMinWidth = 560.0f;
	static constexpr float OuterBorderPadding = 44.0f;
	static constexpr float PlaceholderInnerPadding = 22.0f;
}

void SSpaceshipMainMenuBase::Construct(const FArguments& InArgs)
{
	using namespace SpaceshipCrewSlateMenu;

	World = InArgs._WorldContext;
	OwnerPC = InArgs._OwnerPC;

	ChildSlot
	[
		SNew(SBox)
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		.Padding(FMargin(RootLeftPadding, RootVerticalPadding, 0.0f, RootVerticalPadding))
		[
			SNew(SBox)
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			.MinDesiredWidth(MenuColumnMinWidth)
			[
				SNew(SBorder)
				.Padding(FMargin(OuterBorderPadding))
				[
					SAssignNew(MenuSwitcher, SWidgetSwitcher)
					+ SWidgetSwitcher::Slot()
					[
						BuildMainMenuBody()
					]
					+ SWidgetSwitcher::Slot()
					[
						BuildPlaceholderSlot()
					]
					+ SWidgetSwitcher::Slot()
					[
						SAssignNew(BlueprintPicker, SSpaceshipBlueprintPicker)
						.WorldContext(World)
						.OwnerPC(OwnerPC)
						.OnBack(this, &SSpaceshipMainMenuBase::OnBlueprintPickerBack)
					]
				]
			]
		]
	];
}

TSharedRef<SWidget> SSpaceshipMainMenuBase::BuildPlaceholderSlot()
{
	using namespace SpaceshipCrewSlateMenu;

	return SNew(SBorder)
		.Padding(FMargin(PlaceholderInnerPadding))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Justification(ETextJustify::Center)
				.Font(GetMenuPlaceholderTitleFont())
				.Text_Lambda([this]() { return PlaceholderTitle; })
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin(0.0f, 28.0f, 0.0f, 0.0f))
			[
				SNew(SButton)
				.ContentPadding(FMargin(28.0f, 14.0f))
				.OnClicked(this, &SSpaceshipMainMenuBase::OnBackClicked)
				[
					SNew(STextBlock)
					.Font(GetMenuButtonFont())
					.Text(LOCTEXT("Back", "Назад"))
				]
			]
		];
}

FSlateFontInfo SSpaceshipMainMenuBase::GetMenuTitleFont()
{
	return FCoreStyle::GetDefaultFontStyle("Bold", 38);
}

FSlateFontInfo SSpaceshipMainMenuBase::GetMenuButtonFont()
{
	return FCoreStyle::GetDefaultFontStyle("Regular", 22);
}

FSlateFontInfo SSpaceshipMainMenuBase::GetMenuPlaceholderTitleFont()
{
	return FCoreStyle::GetDefaultFontStyle("Bold", 28);
}

FReply SSpaceshipMainMenuBase::OnRouteClicked(ESpaceshipMenuRoute Route)
{
	using namespace SpaceshipCrewMenu;

	if (IsExitRoute(Route))
	{
		if (UWorld* W = World.Get())
		{
			UKismetSystemLibrary::QuitGame(W, OwnerPC.Get(), EQuitPreference::Quit, false);
		}
		return FReply::Handled();
	}

	if (Route == ESpaceshipMenuRoute::Constructor)
	{
		ShowBlueprintPicker();
		return FReply::Handled();
	}

	PlaceholderTitle = GetDisplayName(Route);
	if (MenuSwitcher.IsValid())
	{
		MenuSwitcher->SetActiveWidgetIndex(PlaceholderSlotIndex);
	}

	return FReply::Handled();
}

FReply SSpaceshipMainMenuBase::OnBackClicked()
{
	ShowMainMenu();
	return FReply::Handled();
}

void SSpaceshipMainMenuBase::OnBlueprintPickerBack()
{
	ShowMainMenu();
}

void SSpaceshipMainMenuBase::ShowBlueprintPicker()
{
	if (BlueprintPicker.IsValid())
	{
		BlueprintPicker->RefreshLists();
	}
	if (MenuSwitcher.IsValid())
	{
		MenuSwitcher->SetActiveWidgetIndex(BlueprintPickerSlotIndex);
	}
}

void SSpaceshipMainMenuBase::ShowMainMenu()
{
	if (MenuSwitcher.IsValid())
	{
		MenuSwitcher->SetActiveWidgetIndex(MainMenuSlotIndex);
	}
}

#undef LOCTEXT_NAMESPACE
