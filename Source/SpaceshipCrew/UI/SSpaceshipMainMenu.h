#pragma once

#include "CoreMinimal.h"
#include "SSpaceshipMainMenuBase.h"

/**
 * Текущая реализация главного меню MVP: список маршрутов из SpaceshipCrewMenu и переходы на заглушки.
 * Собирается в C++ (Construct в базовом классе + BuildMainMenuBody здесь); при смене на UMG можно опереться на USpaceshipCrewMenuWidgetBase.
 */
class SSpaceshipMainMenu : public SSpaceshipMainMenuBase
{
public:
	SLATE_BEGIN_ARGS(SSpaceshipMainMenu) {}

		SLATE_ARGUMENT(TWeakObjectPtr<UWorld>, WorldContext)

		SLATE_ARGUMENT(TWeakObjectPtr<APlayerController>, OwnerPC)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

protected:
	virtual TSharedRef<SWidget> BuildMainMenuBody() override;
};
