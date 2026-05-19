#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ShipBuilder/ShipBlueprintTypes.h"
#include "ShipBlueprintSessionSubsystem.generated.h"

class UShipBlueprintRegistry;

/**
 * Сессия конструктора: выбранный корабль, dirty-флаг и правила сохранения.
 */
UCLASS()
class SPACESHIPCREW_API UShipBlueprintSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "ShipBlueprint")
	FShipBlueprintSession& GetSession() { return Session; }

	UFUNCTION(BlueprintCallable, Category = "ShipBlueprint")
	const FShipBlueprintSession& GetSessionConst() const { return Session; }

	/** Новый пустой корабль. */
	void BeginNewShip(const FText& DisplayName = FText::GetEmpty());

	/** Загрузка для редактирования. */
	bool BeginEditShip(EShipBlueprintSource Source, FName ShipId, FString& OutError);

	void MarkDirty();
	void ClearDirty();

	/** Собрать документ из текущего черновика PC (вызывается из контроллера). */
	void UpdatePendingLayout(const FShipBuilderDraftConfig& Draft, int32 CreditCost);

	/** Применить PendingDocument к черновику. */
	void ApplyPendingToDraft(FShipBuilderDraftConfig& OutDraft) const;

	bool CanSaveInPlace() const;
	bool RequiresSaveAs() const { return Session.bRequiresSaveAsOnWrite; }

	/** Сохранить (перезапись player) или первое сохранение new. */
	bool SaveCurrent(FString& OutError);

	/** Всегда создаёт/перезаписывает player-корабль с указанным id и именем. */
	bool SaveAs(const FName NewShipId, const FText& NewDisplayName, FString& OutError);

	/** Заглушка для T04: последний выбранный корабль для новой игры. */
	UFUNCTION(BlueprintCallable, Category = "ShipBlueprint")
	FShipBlueprintDocument GetSelectedShipForNewGame() const { return SelectedForNewGame; }

	UFUNCTION(BlueprintCallable, Category = "ShipBlueprint")
	void SetSelectedShipForNewGame(const FShipBlueprintDocument& Document) { SelectedForNewGame = Document; }

	/** Устанавливает корабль для новой игры только при успешной ValidateDocumentForPlay. */
	bool TrySetSelectedShipForNewGame(const FShipBlueprintDocument& Document, FString& OutError);

private:
	void UpdateSelectedForNewGameIfPlayReady(const FShipBlueprintDocument& Document);

	UPROPERTY()
	FShipBlueprintSession Session;

	UPROPERTY()
	FShipBlueprintDocument SelectedForNewGame;
};
