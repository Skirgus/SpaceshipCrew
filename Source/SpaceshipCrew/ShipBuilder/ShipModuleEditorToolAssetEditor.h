#pragma once

#if WITH_EDITOR

#include "CoreMinimal.h"
#include "ShipModule/ShipModuleTypes.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "UObject/UnrealType.h"

class IDetailsView;
class SButton;
template<typename ItemType> class SListView;
class SShipModuleEditorToolViewport;
class UShipModuleEditorTool;
class UShipModuleVisualOverride;

/**
 * Кастомный asset editor для UShipModuleEditorTool:
 * - вьюпорт предпросмотра модуля,
 * - Details-панель данных инструмента.
 */
class FShipModuleEditorToolAssetEditor final : public FAssetEditorToolkit
{
public:
	void InitEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UShipModuleEditorTool* InToolAsset);

	// FAssetEditorToolkit
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;

private:
	void RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
	void UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
	TSharedRef<SDockTab> SpawnViewportTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnDetailsTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnPartsTab(const FSpawnTabArgs& Args);
	void ExtendToolbar();
	void ToggleSocketEditMode();
	void OnRefreshPreview();
	void OnDetailsFinishedChanging(const FPropertyChangedEvent& PropertyChangedEvent);
	UShipModuleVisualOverride* ResolveEditableOverride(bool bCreateIfMissing);
	void RebuildPartItems();
	void RebuildSocketItems();
	TSharedRef<ITableRow> GeneratePartRow(TSharedPtr<int32> Item, const TSharedRef<STableViewBase>& OwnerTable);
	TSharedRef<ITableRow> GenerateSocketRow(TSharedPtr<int32> Item, const TSharedRef<STableViewBase>& OwnerTable);
	void OnPartSelectionChanged(TSharedPtr<int32> Item, ESelectInfo::Type SelectInfo);
	void OnSocketSelectionChanged(TSharedPtr<int32> Item, ESelectInfo::Type SelectInfo);
	void SelectPartIndex(int32 NewIndex, bool bFromViewport);
	void SelectSocketIndex(int32 NewIndex);
	FReply OnAddPart();
	FReply OnDuplicatePart();
	FReply OnRemovePart();
	FReply OnAddSocket();
	FReply OnRemoveSocket();
	FReply OnSetSocketType(EShipModuleSocketType NewType);
	FReply OnAttachSocketToPanel(int32 PanelIndex);
	FReply OnUseSelectedAsset();
	FReply OnNudgeSelectedPart(const FVector& Delta);
	FReply OnRotateSelectedPart(float DeltaYaw);
	void HandleViewportDuplicateSelected();
	void HandleViewportDeleteSelected();
	void HandleViewportUseSelectedAsset();
	bool TryGetPanelPlacementData(int32 PanelIndex, FVector& OutCenter, FVector& OutHalfSize) const;
	int32 ResolveNearestPanelIndex(const FShipModuleContactPoint& Socket) const;
	void SnapSocketToPanel(FShipModuleContactPoint& InOutSocket, int32 PanelIndex, bool bAutoType) const;
	void SnapSocketToModuleSurface(FShipModuleContactPoint& InOutSocket) const;
	bool CanPlaceSocketOpening(const FShipModuleContactPoint& Socket) const;
	void SnapAllSocketsToModuleSurface();
	FReply OnPartsDragOver(const FGeometry& Geometry, const FDragDropEvent& DragDropEvent);
	FReply OnPartsDrop(const FGeometry& Geometry, const FDragDropEvent& DragDropEvent);

	static const FName ViewportTabId;
	static const FName DetailsTabId;
	static const FName PartsTabId;

	TObjectPtr<UShipModuleEditorTool> ToolAsset = nullptr;
	TSharedPtr<SShipModuleEditorToolViewport> ViewportWidget;
	TSharedPtr<IDetailsView> DetailsView;
	TSharedPtr<SListView<TSharedPtr<int32>>> PartsListView;
	TSharedPtr<SListView<TSharedPtr<int32>>> SocketsListView;
	TArray<TSharedPtr<int32>> PartItems;
	TArray<TSharedPtr<int32>> SocketItems;
	int32 SelectedPartIndex = INDEX_NONE;
	int32 SelectedSocketIndex = INDEX_NONE;
	bool bSyncingPartSelection = false;
	bool bSyncingSocketSelection = false;
	bool bSocketEditMode = false;
};

#endif // WITH_EDITOR

