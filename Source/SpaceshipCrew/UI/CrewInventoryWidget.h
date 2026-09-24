#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CrewInventoryWidget.generated.h"

class UInventoryComponent;

/**
 * Панель инвентаря (тогл по I / крестик): DnD внутри сетки и на хотбар.
 * Короткий ЛКМ — экипировать; ПКМ — на хотбар без drag.
 */
UCLASS(Blueprintable, BlueprintType)
class SPACESHIPCREW_API UCrewInventoryWidget : public UUserWidget
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

	void RefreshFromInventory();
	void CloseInventory();
	FReply OnSlotClicked(int32 SlotIndex, const FPointerEvent& MouseEvent);
	FReply OnCloseClicked();

	UPROPERTY(Transient)
	TWeakObjectPtr<UInventoryComponent> Inventory;

	TSharedPtr<class SCrewInventoryPanel> Panel;
};
