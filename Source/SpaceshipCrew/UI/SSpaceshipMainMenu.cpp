#include "SSpaceshipMainMenu.h"

#include "Kismet/KismetSystemLibrary.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SpaceshipMainMenu"

void SSpaceshipMainMenu::Construct(const FArguments& InArgs)
{
	World = InArgs._WorldContext;
	OwnerPC = InArgs._OwnerPC;

	ChildSlot
	[
		SNew(SBox)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBorder)
			.Padding(FMargin(32.0f))
			[
				SAssignNew(MenuSwitcher, SWidgetSwitcher)
				+ SWidgetSwitcher::Slot()
				[
					BuildMainPanel()
				]
				+ SWidgetSwitcher::Slot()
				[
					SNew(SBorder)
					.Padding(FMargin(16.0f))
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(STextBlock)
							.Justification(ETextJustify::Center)
							.Text_Lambda([this]() { return PlaceholderTitle; })
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(FMargin(0.0f, 20.0f, 0.0f, 0.0f))
						[
							SNew(SButton)
							.OnClicked(this, &SSpaceshipMainMenu::OnBackClicked)
							[
								SNew(STextBlock)
								.Text(LOCTEXT("Back", "Назад"))
							]
						]
					]
				]
			]
		]
	];
}

TSharedRef<SWidget> SSpaceshipMainMenu::BuildMainPanel()
{
	using namespace SpaceshipCrewMenu;

	TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);

	Box->AddSlot()
	.AutoHeight()
	.Padding(FMargin(0.0f, 0.0f, 0.0f, 16.0f))
	[
		SNew(STextBlock)
		.Justification(ETextJustify::Center)
		.Text(LOCTEXT("GameTitle", "SpaceshipCrew"))
	];

	for (ESpaceshipMenuRoute Route : GetOrderedMenuRoutes())
	{
		const FText Label = GetDisplayName(Route);
		Box->AddSlot()
		.AutoHeight()
		.Padding(FMargin(0.0f, 6.0f, 0.0f, 0.0f))
		[
			SNew(SButton)
			.OnClicked_Lambda([this, Route]()
			{
				return OnRouteClicked(Route);
			})
			[
				SNew(STextBlock)
				.Text(Label)
			]
		];
	}

	return Box;
}

FReply SSpaceshipMainMenu::OnRouteClicked(ESpaceshipMenuRoute Route)
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

	PlaceholderTitle = GetDisplayName(Route);
	if (MenuSwitcher.IsValid())
	{
		MenuSwitcher->SetActiveWidgetIndex(1);
	}

	return FReply::Handled();
}

FReply SSpaceshipMainMenu::OnBackClicked()
{
	if (MenuSwitcher.IsValid())
	{
		MenuSwitcher->SetActiveWidgetIndex(0);
	}
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
