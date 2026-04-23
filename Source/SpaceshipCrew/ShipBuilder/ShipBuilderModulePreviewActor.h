#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShipBuilder/ShipBuilderDraftTypes.h"
#include "ShipModule/ShipModuleTypes.h"
#include "ShipBuilderModulePreviewActor.generated.h"

class UInstancedStaticMeshComponent;
class USceneComponent;
class UStaticMesh;
class UShipModuleCatalog;
class UShipModuleDefinition;

/**
 * Визуальный превью-актор конструктора: собирает геометрию модулей из панелей в одном пространстве.
 *
 * Актор предназначен для MVP-сцены конструктора и использует один и тот же набор панелей
 * как для внешней оболочки, так и для внутреннего объёма (через полую "коробку" модуля).
 * На текущем этапе стыковка модулей линейная (по порядку черновика), а проход между
 * соседними interior-модулями формируется дверной проём с рамкой.
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

private:
	void AddTransformInstance(UInstancedStaticMeshComponent& Component, const FTransform& Transform) const;
	void AddBoxInstance(UInstancedStaticMeshComponent& Component, const FVector& Center, const FVector& Size) const;
	void AddShellPanel(
		UInstancedStaticMeshComponent& NormalComponent,
		UInstancedStaticMeshComponent* DamagedComponent,
		const FVector& Center,
		const FVector& Size,
		int32& InOutPanelOrdinal) const;

	void AddDoorOpeningFrame(
		UInstancedStaticMeshComponent& FrameComponent,
		const FVector& WallCenter,
		float WallThickness,
		float WallSpan,
		float WallHeight,
		bool bNormalAlongX) const;

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
};

