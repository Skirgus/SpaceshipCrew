#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ShipModule/ShipModuleTypes.h"
#include "ShipModuleEditorTool.generated.h"

class UShipModuleDefinition;
class UShipModuleVisualOverride;
class UStaticMesh;

/**
 * Отдельный инструмент редактирования модулей (не привязан к уровню).
 *
 * Создаётся как asset в Content Browser. Кнопки CallInEditor запускают генерацию
 * или обновление VisualOverride для выбранного ModuleDefinition.
 */
UCLASS(BlueprintType)
class SPACESHIPCREW_API UShipModuleEditorTool : public UDataAsset
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	/** Создать/обновить VisualOverride по текущим параметрам TargetModuleDefinition. */
	UFUNCTION(CallInEditor, Category = "ShipBuilder|Editor Tool")
	void GenerateOrUpdateVisualOverride();

	/** Скопировать контактные точки ModuleDefinition в override и включить их использование. */
	UFUNCTION(CallInEditor, Category = "ShipBuilder|Editor Tool")
	void CopyContactPointsToOverride();
#endif

	/** Модуль, который редактируется. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ShipBuilder|Editor Tool")
	TObjectPtr<UShipModuleDefinition> TargetModuleDefinition;

	/** Явно назначенный override (если пусто, создаётся автоматически рядом с ModuleDefinition). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ShipBuilder|Editor Tool")
	TObjectPtr<UShipModuleVisualOverride> TargetVisualOverride;

	/** Базовый меш панели для сгенерированного override. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ShipBuilder|Editor Tool")
	TObjectPtr<UStaticMesh> DefaultPartMesh;

	/** Толщина панелей в сгенерированном override (см). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ShipBuilder|Editor Tool", meta = (ClampMin = "2.0"))
	float PanelThickness = 20.0f;

	/** Перезаписывать текущие VisualParts у override при генерации. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ShipBuilder|Editor Tool")
	bool bOverwriteExistingVisualParts = true;

	/** Автоматически назначать созданный override в TargetModuleDefinition.VisualOverride. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ShipBuilder|Editor Tool")
	bool bAutoAssignOverrideToDefinition = true;

private:
#if WITH_EDITOR
	UShipModuleVisualOverride* ResolveOrCreateOverrideAsset();
	void FillVisualPartsFromDefinition(UShipModuleVisualOverride& OverrideAsset, const UShipModuleDefinition& Definition) const;
	void AddBoxPart(UShipModuleVisualOverride& OverrideAsset, UStaticMesh* Mesh, const FVector& Center, const FVector& Size) const;
	void AddDoorFrame(UShipModuleVisualOverride& OverrideAsset, UStaticMesh* Mesh, const FVector& WallCenter, float WallThickness, float WallSpan, float WallHeight, bool bNormalAlongX) const;
	static bool ShouldForceOpeningForSide(const UShipModuleDefinition& Definition, EShipModuleOpeningSide Side);
#endif
};

