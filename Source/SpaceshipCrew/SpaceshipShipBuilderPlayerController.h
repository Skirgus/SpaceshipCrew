#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ShipBuilderDraftTypes.h"
#include "ShipBuildDomain.h"
#include "SpaceshipCrew.h"
#include "SpaceshipShipBuilderPlayerController.generated.h"

class SSpaceshipShipBuilderRoot;
class UShipModuleCatalog;

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

	void RefreshShipBuilderUi();

private:
	void ToggleCatalog();
	void ToggleChecklist();
	void OnExitPressed();

	FShipBuilderDraftConfig Draft;

	FName HoveredCatalogModuleId = NAME_None;
	bool bCatalogOpen = false;
	bool bChecklistOpen = false;

	TSharedPtr<SSpaceshipShipBuilderRoot> ShipBuilderSlate;
};
