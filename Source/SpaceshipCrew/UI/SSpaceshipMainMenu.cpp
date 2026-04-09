#include "SSpaceshipMainMenu.h"

#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SpaceshipMainMenu"

void SSpaceshipMainMenu::Construct(const FArguments& InArgs)
{
	SSpaceshipMainMenuBase::Construct(
		SSpaceshipMainMenuBase::FArguments()
			.WorldContext(InArgs._WorldContext)
			.OwnerPC(InArgs._OwnerPC));
}

TSharedRef<SWidget> SSpaceshipMainMenu::BuildMainMenuBody()
{
	using namespace SpaceshipCrewMenu;

	TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);

	Box->AddSlot()
	.AutoHeight()
	.Padding(FMargin(0.0f, 0.0f, 0.0f, 24.0f))
	[
		SNew(STextBlock)
		.Justification(ETextJustify::Center)
		.Font(GetMenuTitleFont())
		.Text(LOCTEXT("GameTitle", "SpaceshipCrew"))
	];

	for (ESpaceshipMenuRoute Route : GetOrderedMenuRoutes())
	{
		const FText Label = GetDisplayName(Route);
		Box->AddSlot()
		.AutoHeight()
		.Padding(FMargin(0.0f, 10.0f, 0.0f, 0.0f))
		[
			SNew(SButton)
			.ContentPadding(FMargin(32.0f, 16.0f))
			.OnClicked_Lambda([this, Route]()
			{
				return OnRouteClicked(Route);
			})
			[
				SNew(STextBlock)
				.Font(GetMenuButtonFont())
				.Text(Label)
			]
		];
	}

	return Box;
}

#undef LOCTEXT_NAMESPACE
