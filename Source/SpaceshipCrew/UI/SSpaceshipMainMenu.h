#pragma once

#include "CoreMinimal.h"
#include "SpaceshipCrewMenuRoute.h"
#include "Widgets/SCompoundWidget.h"

class SSpaceshipMainMenu : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSpaceshipMainMenu) {}

		SLATE_ARGUMENT(TWeakObjectPtr<UWorld>, WorldContext)

		SLATE_ARGUMENT(TWeakObjectPtr<APlayerController>, OwnerPC)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	FReply OnRouteClicked(ESpaceshipMenuRoute Route);
	FReply OnBackClicked();

	TSharedRef<SWidget> BuildMainPanel();

	TWeakObjectPtr<UWorld> World;
	TWeakObjectPtr<APlayerController> OwnerPC;

	TSharedPtr<SWidgetSwitcher> MenuSwitcher;
	FText PlaceholderTitle;
};
