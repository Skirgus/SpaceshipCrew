#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ShipBuilder/ShipBuilderDraftTypes.h"
#include "ShipBuilder/ShipBlueprintTypes.h"
#include "ShipModule/ShipBuildDomain.h"
#include "ShipModuleTypes.h"
#include "SpaceshipCrew.h"
#include "SpaceshipShipBuilderPlayerController.generated.h"

class SSpaceshipShipBuilderRoot;
class UShipModuleCatalog;
class UShipBlueprintSessionSubsystem;
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
	virtual void PlayerTick(float DeltaSeconds) override;

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
	void RequestExitToMainMenuForce();

	UShipBlueprintSessionSubsystem* GetBlueprintSession() const;

	FText GetShipSessionTitle() const;
	bool CanSaveShipInPlace() const;
	bool RequiresSaveShipAs() const;
	bool IsShipSessionDirty() const;

	/** Сохранить (перезапись player) или ошибка, если нужен Save As. */
	bool TrySaveShip(FString& OutError);
	bool TrySaveShipAs(const FString& DisplayName, FString& OutError);

	void ApplyLoadedDocumentToDraft();
	bool IsNewShipEditSession() const;
	void NotifyDraftChanged();

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
	void RotatePlacementLeft();
	void RotatePlacementRight();
	void MovePlacementUp();
	void MovePlacementDown();
	void OnRightMouseLookPressed();
	void OnRightMouseLookReleased();
	void OnSelectOrBeginDragPressed();
	void OnEndDragReleased();
	void UpdateRightMouseLook();
	void UpdateModuleDrag();
	void UpdateHoveredModuleUnderCursor();
	bool TrySelectModuleUnderCursor();
	int32 FindDraftModuleIndexByInstanceId(FName InstanceId) const;
	void RecomputeDraftConnectionSockets();
	void RebuildConnectionsFromAdjacency();
	bool IsGridCellOccupied(const FIntVector& Cell, FName IgnoreInstanceId = NAME_None) const;
	FIntVector FindBestSnappedCell(const FIntVector& RawCell, FName MovingInstanceId) const;

	void EnsureCatalogCategoryIndexValid();
	void EnsurePreviewActor();
	void RefreshPreviewFromDraft();
	void SyncLegacyModuleIds();
	void SyncSessionFromDraft();
	FName MakeNextDraftInstanceId() const;
	bool ConfirmDiscardDirtyAndContinue(TFunctionRef<void()> OnConfirmed);

	FShipBuilderDraftConfig Draft;
	int32 PendingPlacementYawStep = 0;
	int32 PendingPlacementZ = 0;
	bool bDraggingModule = false;
	bool bDragMovementActivated = false;
	FVector2D DragStartMousePos = FVector2D::ZeroVector;
	bool bRightMouseLookActive = false;
	FVector2D LastRightMousePos = FVector2D::ZeroVector;
	FName DraggedModuleInstanceId = NAME_None;
	FName SelectedModuleInstanceId = NAME_None;
	FName HoveredModuleInstanceId = NAME_None;
	bool bHasDragTargetCell = false;
	FIntVector DragTargetCell = FIntVector::ZeroValue;

	FName HoveredCatalogModuleId = NAME_None;
	bool bCatalogOpen = false;
	bool bChecklistOpen = false;

	/** Индекс в GetCatalogModuleTypesSorted() для фильтра списка каталога. */
	int32 CatalogCategoryIndex = 0;

	UPROPERTY()
	TObjectPtr<AShipBuilderModulePreviewActor> PreviewActor;

	TSharedPtr<SSpaceshipShipBuilderRoot> ShipBuilderSlate;
};
