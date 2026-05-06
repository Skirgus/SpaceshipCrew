#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ShipBuilder/ShipBuilderDraftTypes.h"
#include "ShipModule/ShipBuildDomain.h"
#include "ShipModuleTypes.h"
#include "SpaceshipCrew.h"
#include "SpaceshipShipBuilderPlayerController.generated.h"

class SSpaceshipShipBuilderRoot;
class UShipModuleCatalog;
class AShipBuilderModulePreviewActor;

/**
 * Конструктор корабля: ввод, черновик модулей, расчёт параметров и валидация T02b по автоматической цепочке.
 */
UCLASS()
class SPACESHIPCREW_API ASpaceshipShipBuilderPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void SetupInputComponent() override;

	/** Черновик для UI и расчётов. */
	FShipBuilderDraftConfig& AccessDraft() { return Draft; }

	UShipModuleCatalog* GetModuleCatalog() const;

	FShipBuildValidationResult ComputeValidation() const;

	void SetHoveredCatalogModule(FName ModuleId);
	FName GetHoveredCatalogModule() const { return HoveredCatalogModuleId; }

	void SetCatalogOpen(const bool bOpen) { bCatalogOpen = bOpen; }
	bool IsCatalogOpen() const { return bCatalogOpen; }

	void SetChecklistOpen(const bool bOpen) { bChecklistOpen = bOpen; }
	bool IsChecklistOpen() const { return bChecklistOpen; }

	void AppendModuleToDraft(FName ModuleId);
	void RequestExitToMainMenu();

	/** Сумма эффективной стоимости модулей в черновике (CreditCost или масса). */
	int32 GetDraftTotalCreditCost() const;

	void RefreshShipBuilderUi();

	/** Типы модулей, которые реально есть в каталоге, по возрастанию enum. */
	TArray<EShipModuleType> GetCatalogModuleTypesSorted() const;

	int32 GetCatalogCategoryIndex() const { return CatalogCategoryIndex; }

	/** Переключение категории в открытом каталоге (Q — предыдущая, T — следующая). */
	void CycleCatalogCategory(int32 Delta);

private:
	void ToggleCatalog();
	void ToggleChecklist();
	void OnExitPressed();
	void CatalogCyclePrev();
	void CatalogCycleNext();

	void EnsureCatalogCategoryIndexValid();
	void EnsurePreviewActor();
	void RefreshPreviewFromDraft();

	FShipBuilderDraftConfig Draft;

	FName HoveredCatalogModuleId = NAME_None;
	bool bCatalogOpen = false;
	bool bChecklistOpen = false;

	/** Индекс в GetCatalogModuleTypesSorted() для фильтра списка каталога. */
	int32 CatalogCategoryIndex = 0;

	UPROPERTY()
	TObjectPtr<AShipBuilderModulePreviewActor> PreviewActor;

	TSharedPtr<SSpaceshipShipBuilderRoot> ShipBuilderSlate;
};
