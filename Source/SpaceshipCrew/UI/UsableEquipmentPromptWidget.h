#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UsableEquipmentPromptWidget.generated.h"

class SUsableEquipmentCallout;

/**
 * Подсказка взаимодействия у предмета: белая плашка с именем,
 * действие и клавиша справа, линия к точке на объекте.
 */
UCLASS(Blueprintable, BlueprintType)
class SPACESHIPCREW_API UUsableEquipmentPromptWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void SetCallout(
		const FText& InTitle,
		const FText& InAction,
		FVector2D InAnchorSlate,
		bool bInOnScreen,
		bool bInShowAction);

	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	FText Title;
	FText Action;
	FVector2D AnchorSlate = FVector2D::ZeroVector;
	bool bOnScreen = false;
	bool bShowAction = false;
	TSharedPtr<SUsableEquipmentCallout> Callout;
};
