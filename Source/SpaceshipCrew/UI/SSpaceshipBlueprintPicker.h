#pragma once

#include "CoreMinimal.h"
#include "ShipBuilder/ShipBlueprintTypes.h"
#include "Widgets/SCompoundWidget.h"

/**
 * Экран выбора корабля перед входом в конструктор (T03).
 */
class SSpaceshipBlueprintPicker : public SCompoundWidget
{
public:
	DECLARE_DELEGATE(FOnPickerBack);
	DECLARE_DELEGATE(FOnPickerConfirmed);

	SLATE_BEGIN_ARGS(SSpaceshipBlueprintPicker) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UWorld>, WorldContext)
		SLATE_ARGUMENT(TWeakObjectPtr<APlayerController>, OwnerPC)
		SLATE_EVENT(FOnPickerBack, OnBack)
		SLATE_EVENT(FOnPickerConfirmed, OnConfirmed)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void RefreshLists();

private:
	FReply OnBackClicked();
	FReply OnNewShipClicked();
	FReply OnEntryClicked(EShipBlueprintSource Source, FName ShipId);

	void BuildEntryList(
		const TArray<FShipBlueprintListEntry>& Entries,
		TSharedRef<class SVerticalBox> TargetBox);

	void ShowPickerError(const FText& Message) const;

	TWeakObjectPtr<UWorld> World;
	TWeakObjectPtr<APlayerController> OwnerPC;
	FOnPickerBack OnBack;
	FOnPickerConfirmed OnConfirmed;

	TSharedPtr<class SVerticalBox> ProjectListBox;
	TSharedPtr<class SVerticalBox> PlayerListBox;
};
