#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShipBuilder/ShipBuilderDraftTypes.h"
#include "ShipModule/ShipModuleTypes.h"
#include "ShipBuilderModulePreviewActor.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInterface;
class USceneComponent;
class UStaticMesh;
class UShipModuleCatalog;
class UShipModuleDefinition;

/**
 * Визуальный превью-актор конструктора: собирает геометрию модулей из панелей в одном пространстве.
 *
 * VisualOverride с VisualParts заменяет procedural shell; стены с WallSocketName при стыковке
 * меняются на OpeningMesh (Passage / SlidingDoor). Иначе — полая коробка с проёмами по связям.
 */
UCLASS()
class SPACESHIPCREW_API AShipBuilderModulePreviewActor : public AActor
{
	GENERATED_BODY()

public:
	AShipBuilderModulePreviewActor();

	/**
	 * Полная пересборка превью из текущего черновика конструктора.
	 *
	 * @param Draft Черновик модулей в порядке установки.
	 * @param Catalog Каталог определений модулей.
	 */
	void RebuildFromDraft(const FShipBuilderDraftConfig& Draft, const UShipModuleCatalog& Catalog);
	void SetSelectedModuleInstanceId(FName InstanceId) { SelectedModuleInstanceId = InstanceId; }
	void SetHoveredModuleInstanceId(FName InstanceId) { HoveredModuleInstanceId = InstanceId; }
	void SetDragGhostTarget(bool bEnabled, FName InstanceId, FIntVector GridCorner)
	{
		bShowDragGhost = bEnabled;
		DragGhostInstanceId = InstanceId;
		DragGhostGridPos = GridCorner;
	}

	/** Включает/выключает демонстрационный режим повреждённых панелей. */
	void SetPreviewDamageEnabled(bool bEnabled) { bPreviewDamage = bEnabled; }
	bool IsPreviewDamageEnabled() const { return bPreviewDamage; }

protected:
	UPROPERTY(VisibleAnywhere, Category = "ShipBuilder|Preview")
	TObjectPtr<USceneComponent> Root;

	/** Пулы инстансов по мешам для обычных панелей оболочки. */
	TMap<TObjectPtr<UStaticMesh>, TObjectPtr<UInstancedStaticMeshComponent>> PanelMeshPools;

	/** Пулы инстансов по мешам для повреждённых панелей. */
	TMap<TObjectPtr<UStaticMesh>, TObjectPtr<UInstancedStaticMeshComponent>> DamagedPanelMeshPools;

	/** Пулы инстансов по мешам для рамок дверных проёмов. */
	TMap<TObjectPtr<UStaticMesh>, TObjectPtr<UInstancedStaticMeshComponent>> DoorFrameMeshPools;

	/** Пулы инстансов по мешам для цельных блоков без интерьера. */
	TMap<TObjectPtr<UStaticMesh>, TObjectPtr<UInstancedStaticMeshComponent>> SolidMeshPools;

	/** Пулы инстансов по мешам для ручных override-элементов. */
	TMap<TObjectPtr<UStaticMesh>, TObjectPtr<UInstancedStaticMeshComponent>> OverrideMeshPools;
	TMap<TObjectPtr<UStaticMesh>, TObjectPtr<UInstancedStaticMeshComponent>> SelectionMeshPools;
	TMap<TObjectPtr<UStaticMesh>, TObjectPtr<UInstancedStaticMeshComponent>> SocketMarkerPools;

	/** Зазор между модулями в линейной цепочке (см). */
	UPROPERTY(EditAnywhere, Category = "ShipBuilder|Preview", meta = (ClampMin = "0.0"))
	float ModuleGap = 0.0f;

	/** Толщина панелей оболочки (см). */
	UPROPERTY(EditAnywhere, Category = "ShipBuilder|Preview", meta = (ClampMin = "2.0"))
	float PanelThickness = 20.0f;

	/** Меш панели (по умолчанию /Engine/BasicShapes/Cube). */
	UPROPERTY(EditAnywhere, Category = "ShipBuilder|Preview")
	TObjectPtr<UStaticMesh> PanelMesh;

	/** Меш дверной рамки (если не задан, используется PanelMesh). */
	UPROPERTY(EditAnywhere, Category = "ShipBuilder|Preview")
	TObjectPtr<UStaticMesh> DoorFrameMesh;

	/** Меш повреждённой панели (если не задан, используется PanelMesh). */
	UPROPERTY(EditAnywhere, Category = "ShipBuilder|Preview")
	TObjectPtr<UStaticMesh> DamagedPanelMesh;

	/** Базовый меш для solid-модулей без интерьера. */
	UPROPERTY(EditAnywhere, Category = "ShipBuilder|Preview")
	TObjectPtr<UStaticMesh> SolidModuleMesh;

	/** Материал socket-маркеров (используется для явного зеленого цвета). */
	UPROPERTY(EditAnywhere, Category = "ShipBuilder|Preview|Selection")
	TObjectPtr<UMaterialInterface> SocketMarkerBaseMaterial;

	/** Опциональные переопределения меша панелей по типу модуля. */
	UPROPERTY(EditAnywhere, Category = "ShipBuilder|Preview")
	TMap<EShipModuleType, TObjectPtr<UStaticMesh>> ModuleTypePanelMeshOverrides;

	/** Опциональные переопределения меша solid-блока по типу модуля. */
	UPROPERTY(EditAnywhere, Category = "ShipBuilder|Preview")
	TMap<EShipModuleType, TObjectPtr<UStaticMesh>> ModuleTypeSolidMeshOverrides;

	/** Деморежим: часть панелей рендерится как повреждённые. */
	UPROPERTY(EditAnywhere, Category = "ShipBuilder|Preview|Damage")
	bool bPreviewDamage = false;

	/** Каждая N-я панель отображается в повреждённом состоянии. */
	UPROPERTY(EditAnywhere, Category = "ShipBuilder|Preview|Damage", meta = (ClampMin = "2"))
	int32 DamageEveryNthPanel = 6;

	/** Толщина рамки выделения модуля (см). */
	UPROPERTY(EditAnywhere, Category = "ShipBuilder|Preview|Selection", meta = (ClampMin = "1.0"))
	float SelectionOutlineThickness = 8.0f;

	/** Зазор рамки выделения от габарита модуля (см). */
	UPROPERTY(EditAnywhere, Category = "ShipBuilder|Preview|Selection", meta = (ClampMin = "0.0"))
	float SelectionOutlinePadding = 12.0f;

	UPROPERTY(EditAnywhere, Category = "ShipBuilder|Preview|Selection")
	FLinearColor SelectionColor = FLinearColor(1.0f, 0.86f, 0.15f, 1.0f);

	UPROPERTY(EditAnywhere, Category = "ShipBuilder|Preview|Selection")
	FLinearColor SocketMarkerColor = FLinearColor(0.0f, 1.0f, 0.2f, 1.0f);

	UPROPERTY(EditAnywhere, Category = "ShipBuilder|Preview|Selection", meta = (ClampMin = "5.0"))
	float SocketMarkerSize = 36.0f;

	UPROPERTY(EditAnywhere, Category = "ShipBuilder|Preview|Selection", meta = (ClampMin = "0.0"))
	float SocketMarkerOffset = 12.0f;

private:
	void AddTransformInstance(UInstancedStaticMeshComponent& Component, const FTransform& Transform) const;
	void AddBoxInstance(UInstancedStaticMeshComponent& Component, const FVector& Center, const FVector& Size) const;
	/** Инстансы маркеров сокетов; при пустом GatherEffective подставляет шесть граней по Size только для отрисовки. */
	void AddEffectiveSocketMarkerInstances(
		const UShipModuleDefinition& Def,
		const FTransform& ModuleTransform,
		UInstancedStaticMeshComponent& SocketPool,
		float MarkerSize,
		float InSocketMarkerOffset) const;
	void AddShellPanel(
		UInstancedStaticMeshComponent& NormalComponent,
		UInstancedStaticMeshComponent* DamagedComponent,
		const FVector& Center,
		const FVector& Size,
		int32& InOutPanelOrdinal) const;

	void AddDoorOpeningFrame(
		UInstancedStaticMeshComponent& FrameComponent,
		const FRotator& ModuleYaw,
		const FVector& ModuleCenter,
		const FVector& LocalWallCenter,
		float WallThickness,
		float WallSpan,
		float WallHeight,
		bool bNormalAlongX) const;

	/** Рамка люка в горизонтальной панели пола/потолка (нормаль по Z). */
	void AddPanelHatchFrame(
		UInstancedStaticMeshComponent& FrameComponent,
		const FRotator& ModuleYaw,
		const FVector& ModuleCenter,
		const FVector& LocalPanelCenter,
		float Thickness,
		float HatchSpan) const;

	void AddRotatedSelectionOutline(
		UInstancedStaticMeshComponent& SelectionPool,
		const FRotator& ModuleYaw,
		const FVector& Center,
		const FVector& Size,
		float Pad,
		float Thickness) const;

	static bool ShouldForceOpeningForSide(const UShipModuleDefinition& Definition, EShipModuleOpeningSide Side);

	UInstancedStaticMeshComponent& GetOrCreatePool(
		TMap<TObjectPtr<UStaticMesh>, TObjectPtr<UInstancedStaticMeshComponent>>& Pools,
		UStaticMesh* Mesh,
		const TCHAR* NamePrefix);

	static UStaticMesh* ResolveMeshForType(
		const TMap<EShipModuleType, TObjectPtr<UStaticMesh>>& Overrides,
		const UStaticMesh* DefaultMesh,
		EShipModuleType ModuleType);

	void ClearPools(TMap<TObjectPtr<UStaticMesh>, TObjectPtr<UInstancedStaticMeshComponent>>& Pools);
	void ConfigureSelectionComponent(UInstancedStaticMeshComponent& Component) const;
	void ConfigureSocketMarkerComponent(UInstancedStaticMeshComponent& Component) const;
	FVector GetDragGhostWorldCenter(const FVector& ModuleSize) const;

	FName SelectedModuleInstanceId = NAME_None;
	FName HoveredModuleInstanceId = NAME_None;
	bool bShowDragGhost = false;
	FName DragGhostInstanceId = NAME_None;
	FIntVector DragGhostGridPos = FIntVector::ZeroValue;
};

