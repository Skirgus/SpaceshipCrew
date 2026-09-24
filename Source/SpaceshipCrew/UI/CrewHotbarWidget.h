#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CrewHotbarWidget.generated.h"

class UInventoryComponent;
class FCrewInventoryDragDropOp;

/**
 * Нижняя панель быстрого доступа: drop из инвентаря, drag наружу снимает слот.
 */
UCLASS(Blueprintable, BlueprintType)
class SPACESHIPCREW_API UCrewHotbarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void BindInventory(UInventoryComponent* InInventory);

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeDestruct() override;

private:
	UFUNCTION()
	void HandleInventoryChanged();

	UFUNCTION()
	void HandleHotbarChanged();

	void RefreshFromInventory();
	bool HandleHotbarDrop(TSharedPtr<FCrewInventoryDragDropOp> Op, int32 TargetHotbarIndex);

	UPROPERTY(Transient)
	TWeakObjectPtr<UInventoryComponent> Inventory;

	TSharedPtr<class SCrewHotbarPanel> Panel;
};
