#pragma once

#include "CoreMinimal.h"
#include "SpaceshipCrewMenuRoute.h"
#include "Widgets/SCompoundWidget.h"

class SWidgetSwitcher;

/**
 * Базовый Slate-виджет главного меню: колонка слева, крупная типографика, переключатель
 * «список кнопок» ↔ «заглушка раздела». Конкретный набор пунктов задаётся в наследнике через BuildMainMenuBody().
 */
class SSpaceshipMainMenuBase : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSpaceshipMainMenuBase) {}

		/** Мир для QuitGame и контекста. */
		SLATE_ARGUMENT(TWeakObjectPtr<UWorld>, WorldContext)

		/** Локальный PlayerController (владелец ввода и выхода из игры). */
		SLATE_ARGUMENT(TWeakObjectPtr<APlayerController>, OwnerPC)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

protected:
	/** Панель с кнопками главного экрана (реализуется в производном классе, например SSpaceshipMainMenu). */
	virtual TSharedRef<SWidget> BuildMainMenuBody() = 0;

	FReply OnRouteClicked(ESpaceshipMenuRoute Route);
	FReply OnBackClicked();
	void OnBlueprintPickerBack();

	void ShowBlueprintPicker();
	void ShowMainMenu();

	TSharedRef<SWidget> BuildPlaceholderSlot();

	TWeakObjectPtr<UWorld> World;
	TWeakObjectPtr<APlayerController> OwnerPC;

	TSharedPtr<SWidgetSwitcher> MenuSwitcher;
	TSharedPtr<class SSpaceshipBlueprintPicker> BlueprintPicker;
	FText PlaceholderTitle;

	static constexpr int32 MainMenuSlotIndex = 0;
	static constexpr int32 PlaceholderSlotIndex = 1;
	static constexpr int32 BlueprintPickerSlotIndex = 2;

protected:
	static FSlateFontInfo GetMenuTitleFont();
	static FSlateFontInfo GetMenuButtonFont();
	static FSlateFontInfo GetMenuPlaceholderTitleFont();
};
