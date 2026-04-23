#pragma once

#if WITH_EDITOR

#include "CoreMinimal.h"
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
	void OnRefreshPreview();
	void OnDetailsFinishedChanging(const FPropertyChangedEvent& PropertyChangedEvent);
	UShipModuleVisualOverride* ResolveEditableOverride(bool bCreateIfMissing);
	void RebuildPartItems();
	TSharedRef<ITableRow> GeneratePartRow(TSharedPtr<int32> Item, const TSharedRef<STableViewBase>& OwnerTable);
	void OnPartSelectionChanged(TSharedPtr<int32> Item, ESelectInfo::Type SelectInfo);
	void SelectPartIndex(int32 NewIndex, bool bFromViewport);
	FReply OnAddPart();
	FReply OnDuplicatePart();
	FReply OnRemovePart();
	FReply OnUseSelectedAsset();
	FReply OnNudgeSelectedPart(const FVector& Delta);
	FReply OnRotateSelectedPart(float DeltaYaw);
	void HandleViewportDuplicateSelected();
	void HandleViewportDeleteSelected();
	void HandleViewportUseSelectedAsset();
	FReply OnPartsDragOver(const FGeometry& Geometry, const FDragDropEvent& DragDropEvent);
	FReply OnPartsDrop(const FGeometry& Geometry, const FDragDropEvent& DragDropEvent);

	static const FName ViewportTabId;
	static const FName DetailsTabId;
	static const FName PartsTabId;

	TObjectPtr<UShipModuleEditorTool> ToolAsset = nullptr;
	TSharedPtr<SShipModuleEditorToolViewport> ViewportWidget;
	TSharedPtr<IDetailsView> DetailsView;
	TSharedPtr<SListView<TSharedPtr<int32>>> PartsListView;
	TArray<TSharedPtr<int32>> PartItems;
	int32 SelectedPartIndex = INDEX_NONE;
	bool bSyncingPartSelection = false;
};

#endif // WITH_EDITOR

