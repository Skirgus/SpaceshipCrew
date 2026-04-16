#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SWidget.h"

class ASpaceshipShipBuilderPlayerController;
class SScrollBox;

/**
 * Оверлей конструктора корабля (T02c-1): раскладка как на референсе — углы, нижняя полоса, панели поверх сцены.
 */
class SSpaceshipShipBuilderRoot : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSpaceshipShipBuilderRoot) {}

		SLATE_ARGUMENT(TWeakObjectPtr<ASpaceshipShipBuilderPlayerController>, OwnerPC)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SSpaceshipShipBuilderRoot() override;

	void RequestRefresh();

	/** Полная пересборка правой панели каталога (тип, сегменты, список модулей). */
	void RebuildCatalogPanel();

private:
	void RebuildCatalogList();

	TSharedRef<class SWidget> BuildFooterStatsRow() const;

	FText GetFooterValueText(FName ParameterId, const FText& Placeholder) const;
	FSlateColor GetFooterValueColor(FName ParameterId) const;

	FText GetModuleInspectTitleText() const;
	FText GetModuleInspectBodyText() const;

	TWeakObjectPtr<ASpaceshipShipBuilderPlayerController> OwnerPC;

	TSharedPtr<class SVerticalBox> CatalogPanelRoot;
	TSharedPtr<SScrollBox> CatalogList;
};
