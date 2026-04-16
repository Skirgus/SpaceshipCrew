#pragma once

#include "CoreMinimal.h"
#include "Widgets/Layout/SVerticalBox.h"
#include "Widgets/SCompoundWidget.h"

class ASpaceshipShipBuilderPlayerController;

/**
 * Полноэкранный Slate UI конструктора (T02c-1), стиль близок к главному меню: тёмный фон, акцентный текст.
 * Данные читаются с PlayerController через лямбды (без дублирования состояния).
 */
class SSpaceshipShipBuilderRoot : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSpaceshipShipBuilderRoot) {}

		SLATE_ARGUMENT(TWeakObjectPtr<ASpaceshipShipBuilderPlayerController>, OwnerPC)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Явное обновление после изменения черновика (инвалидация виджета). */
	void RequestRefresh();

private:
	void RebuildCatalogList();

	TWeakObjectPtr<ASpaceshipShipBuilderPlayerController> OwnerPC;
	TSharedPtr<SVerticalBox> CatalogList;
};
