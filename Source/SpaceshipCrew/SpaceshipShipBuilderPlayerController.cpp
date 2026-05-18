#include "SpaceshipShipBuilderPlayerController.h"

#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Kismet/GameplayStatics.h"
#include "ShipBuilder/ShipBuilderModulePreviewActor.h"
#include "ShipBuilder/ShipBuilderDomainGlue.h"
#include "ShipModuleCatalog.h"
#include "ShipModuleDefinition.h"
#include "ShipModuleTypes.h"
#include "UI/SSpaceshipShipBuilderRoot.h"
#include "Menu/SpaceshipCrewLevelTravel.h"
#include "ShipBuilder/ShipBlueprintNaming.h"
#include "ShipBuilder/ShipBlueprintRegistry.h"
#include "ShipBuilder/ShipBlueprintSessionSubsystem.h"
#include "ShipBuilder/ShipBlueprintTypes.h"
#include "Misc/MessageDialog.h"

namespace SpaceshipShipBuilderInputPrivate
{
	static constexpr float DragGridStepXY = 400.0f;
	static constexpr float DragGridStepZ = 300.0f;
	static constexpr float SelectionScreenRadiusPx = 140.0f;
}

void ASpaceshipShipBuilderPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalPlayerController())
	{
		return;
	}

	ApplyLoadedDocumentToDraft();

	bShowMouseCursor = true;
	FInputModeGameAndUI Mode;
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	Mode.SetHideCursorDuringCapture(false);
	SetInputMode(Mode);
	if (UWorld* World = GetWorld())
	{
		EnsurePreviewActor();
		RefreshPreviewFromDraft();

		if (UGameViewportClient* ViewportClient = World->GetGameViewport())
		{
			ShipBuilderSlate = SNew(SSpaceshipShipBuilderRoot).OwnerPC(this);
			ViewportClient->AddViewportWidgetContent(ShipBuilderSlate.ToSharedRef(), 20);
			ShipBuilderSlate->RequestRefresh();
		}
	}
}

void ASpaceshipShipBuilderPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (PreviewActor)
	{
		PreviewActor->Destroy();
		PreviewActor = nullptr;
	}

	if (ShipBuilderSlate.IsValid())
	{
		if (UWorld* World = GetWorld())
		{
			if (UGameViewportClient* ViewportClient = World->GetGameViewport())
			{
				ViewportClient->RemoveViewportWidgetContent(ShipBuilderSlate.ToSharedRef());
			}
		}
		ShipBuilderSlate.Reset();
	}
	Super::EndPlay(EndPlayReason);
}

void ASpaceshipShipBuilderPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (!InputComponent)
	{
		return;
	}
	InputComponent->BindKey(EKeys::G, IE_Pressed, this, &ASpaceshipShipBuilderPlayerController::ToggleCatalog);
	InputComponent->BindKey(EKeys::C, IE_Pressed, this, &ASpaceshipShipBuilderPlayerController::ToggleChecklist);
	InputComponent->BindKey(EKeys::Tab, IE_Pressed, this, &ASpaceshipShipBuilderPlayerController::OnExitPressed);
	InputComponent->BindKey(EKeys::Q, IE_Pressed, this, &ASpaceshipShipBuilderPlayerController::CatalogCyclePrev);
	InputComponent->BindKey(EKeys::T, IE_Pressed, this, &ASpaceshipShipBuilderPlayerController::CatalogCycleNext);
	InputComponent->BindKey(EKeys::R, IE_Pressed, this, &ASpaceshipShipBuilderPlayerController::RotatePlacementRight);
	InputComponent->BindKey(EKeys::E, IE_Pressed, this, &ASpaceshipShipBuilderPlayerController::RotatePlacementLeft);
	InputComponent->BindKey(EKeys::PageUp, IE_Pressed, this, &ASpaceshipShipBuilderPlayerController::MovePlacementUp);
	InputComponent->BindKey(EKeys::PageDown, IE_Pressed, this, &ASpaceshipShipBuilderPlayerController::MovePlacementDown);
	InputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &ASpaceshipShipBuilderPlayerController::OnRightMouseLookPressed);
	InputComponent->BindKey(EKeys::RightMouseButton, IE_Released, this, &ASpaceshipShipBuilderPlayerController::OnRightMouseLookReleased);
	InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &ASpaceshipShipBuilderPlayerController::OnSelectOrBeginDragPressed);
	InputComponent->BindKey(EKeys::LeftMouseButton, IE_Released, this, &ASpaceshipShipBuilderPlayerController::OnEndDragReleased);
}

void ASpaceshipShipBuilderPlayerController::PlayerTick(float DeltaSeconds)
{
	Super::PlayerTick(DeltaSeconds);
	UpdateRightMouseLook();
	UpdateHoveredModuleUnderCursor();
	UpdateModuleDrag();
}

TArray<EShipModuleType> ASpaceshipShipBuilderPlayerController::GetCatalogModuleTypesSorted() const
{
	TSet<EShipModuleType> Seen;
	if (const UShipModuleCatalog* Cat = GetModuleCatalog())
	{
		for (UShipModuleDefinition* Def : Cat->GetAllModules())
		{
			if (Def)
			{
				Seen.Add(Def->ModuleType);
			}
		}
	}
	TArray<EShipModuleType> Out = Seen.Array();
	Out.Sort([](const EShipModuleType& A, const EShipModuleType& B)
	{
		return static_cast<uint8>(A) < static_cast<uint8>(B);
	});
	return Out;
}

void ASpaceshipShipBuilderPlayerController::EnsureCatalogCategoryIndexValid()
{
	const TArray<EShipModuleType> Types = GetCatalogModuleTypesSorted();
	if (Types.Num() == 0)
	{
		CatalogCategoryIndex = 0;
		return;
	}
	CatalogCategoryIndex = FMath::Clamp(CatalogCategoryIndex, 0, Types.Num() - 1);
}

void ASpaceshipShipBuilderPlayerController::CycleCatalogCategory(const int32 Delta)
{
	if (!bCatalogOpen || Delta == 0)
	{
		return;
	}
	const TArray<EShipModuleType> Types = GetCatalogModuleTypesSorted();
	if (Types.Num() == 0)
	{
		return;
	}
	const int32 N = Types.Num();
	CatalogCategoryIndex = ((CatalogCategoryIndex + Delta) % N + N) % N;
	EnsureCatalogCategoryIndexValid();
	if (ShipBuilderSlate.IsValid())
	{
		ShipBuilderSlate->RebuildCatalogPanel();
	}
	RefreshShipBuilderUi();
}

void ASpaceshipShipBuilderPlayerController::CatalogCyclePrev()
{
	CycleCatalogCategory(-1);
}

void ASpaceshipShipBuilderPlayerController::CatalogCycleNext()
{
	CycleCatalogCategory(1);
}

UShipModuleCatalog* ASpaceshipShipBuilderPlayerController::GetModuleCatalog() const
{
	if (UGameInstance* GI = GetGameInstance())
	{
		return GI->GetSubsystem<UShipModuleCatalog>();
	}
	return nullptr;
}

FShipBuildValidationResult ASpaceshipShipBuilderPlayerController::ComputeValidation() const
{
	const UShipModuleCatalog* Catalog = GetModuleCatalog();
	FShipBuildValidationResult Result;
	if (!Catalog)
	{
		Result.bIsValid = false;
		Result.Errors.Add(TEXT("Каталог модулей недоступен."));
		return Result;
	}

	FCatalogShipBuildModuleResolver Resolver(*Catalog);
	FShipBuildDomainModel Model(Resolver);
	FString Error;
	if (!SpaceshipCrew_BuildDomainFromDraftChain(Draft, Resolver, Model, Error))
	{
		Result.bIsValid = false;
		Result.Errors.Add(Error);
		return Result;
	}
	return Model.Validate();
}

void ASpaceshipShipBuilderPlayerController::SetHoveredCatalogModule(const FName ModuleId)
{
	if (HoveredCatalogModuleId == ModuleId)
	{
		return;
	}
	HoveredCatalogModuleId = ModuleId;
	if (ShipBuilderSlate.IsValid())
	{
		ShipBuilderSlate->Invalidate(EInvalidateWidgetReason::LayoutAndVolatility);
	}
}

int32 ASpaceshipShipBuilderPlayerController::GetDraftTotalCreditCost() const
{
	const UShipModuleCatalog* Catalog = GetModuleCatalog();
	if (!Catalog)
	{
		return 0;
	}
	int32 Sum = 0;
	for (const FName Id : Draft.ModuleIds)
	{
		if (const UShipModuleDefinition* Mod = Catalog->FindModuleById(Id))
		{
			Sum += Mod->GetEffectiveCreditCost();
		}
	}
	return Sum;
}

void ASpaceshipShipBuilderPlayerController::AppendModuleToDraft(const FName ModuleId)
{
	if (ModuleId.IsNone())
	{
		return;
	}

	const FName NewInstanceId = MakeNextDraftInstanceId();
	FShipBuilderDraftConfig::FPlacedModule NewModule;
	NewModule.InstanceId = NewInstanceId;
	NewModule.ModuleId = ModuleId;
	NewModule.YawStep = ((PendingPlacementYawStep % 4) + 4) % 4;
	NewModule.GridPos = FIntVector(Draft.PlacedModules.Num(), 0, PendingPlacementZ);
	Draft.PlacedModules.Add(NewModule);

	RebuildConnectionsFromAdjacency();

	SyncLegacyModuleIds();
	NotifyDraftChanged();
	RefreshPreviewFromDraft();
	RefreshShipBuilderUi();
}

UShipBlueprintSessionSubsystem* ASpaceshipShipBuilderPlayerController::GetBlueprintSession() const
{
	if (UGameInstance* GI = GetGameInstance())
	{
		return GI->GetSubsystem<UShipBlueprintSessionSubsystem>();
	}
	return nullptr;
}

void ASpaceshipShipBuilderPlayerController::ApplyLoadedDocumentToDraft()
{
	if (UShipBlueprintSessionSubsystem* Session = GetBlueprintSession())
	{
		Session->ApplyPendingToDraft(Draft);
		SyncLegacyModuleIds();

		// Шаблоны/старые JSON без Connections: восстановить стыковки по соседству на сетке.
		if (Draft.Connections.Num() == 0 && Draft.PlacedModules.Num() >= 2)
		{
			RebuildConnectionsFromAdjacency();
			SyncLegacyModuleIds();
		}

		SyncSessionFromDraft();
	}
}

void ASpaceshipShipBuilderPlayerController::SyncSessionFromDraft()
{
	if (UShipBlueprintSessionSubsystem* Session = GetBlueprintSession())
	{
		Session->UpdatePendingLayout(Draft, GetDraftTotalCreditCost());
	}
}

void ASpaceshipShipBuilderPlayerController::NotifyDraftChanged()
{
	if (UShipBlueprintSessionSubsystem* Session = GetBlueprintSession())
	{
		Session->UpdatePendingLayout(Draft, GetDraftTotalCreditCost());
		Session->MarkDirty();
	}
}

bool ASpaceshipShipBuilderPlayerController::IsNewShipEditSession() const
{
	if (const UShipBlueprintSessionSubsystem* Session = GetBlueprintSession())
	{
		return Session->GetSessionConst().EditSource == EShipBlueprintEditSource::New;
	}
	return false;
}

FText ASpaceshipShipBuilderPlayerController::GetShipSessionTitle() const
{
	if (const UShipBlueprintSessionSubsystem* Session = GetBlueprintSession())
	{
		const FShipBlueprintSession& S = Session->GetSessionConst();
		if (!S.DisplayName.IsEmpty())
		{
			return S.DisplayName;
		}
		if (!S.SourceShipId.IsNone())
		{
			return FText::FromName(S.SourceShipId);
		}
	}
	return NSLOCTEXT("SpaceshipCrew", "NewShipTitle", "Новый корабль");
}

bool ASpaceshipShipBuilderPlayerController::CanSaveShipInPlace() const
{
	if (const UShipBlueprintSessionSubsystem* Session = GetBlueprintSession())
	{
		return Session->CanSaveInPlace();
	}
	return false;
}

bool ASpaceshipShipBuilderPlayerController::RequiresSaveShipAs() const
{
	if (const UShipBlueprintSessionSubsystem* Session = GetBlueprintSession())
	{
		return Session->RequiresSaveAs();
	}
	return true;
}

bool ASpaceshipShipBuilderPlayerController::IsShipSessionDirty() const
{
	if (const UShipBlueprintSessionSubsystem* Session = GetBlueprintSession())
	{
		return Session->GetSessionConst().bDirty;
	}
	return false;
}

bool ASpaceshipShipBuilderPlayerController::TrySaveShip(FString& OutError)
{
	if (UShipBlueprintSessionSubsystem* Session = GetBlueprintSession())
	{
		SyncSessionFromDraft();
		if (Session->RequiresSaveAs())
		{
			OutError = TEXT("Используйте «Сохранить как…».");
			return false;
		}
		return Session->SaveCurrent(OutError);
	}
	OutError = TEXT("Сессия недоступна.");
	return false;
}

bool ASpaceshipShipBuilderPlayerController::TrySaveShipAs(const FString& DisplayName, FString& OutError)
{
	if (DisplayName.IsEmpty())
	{
		OutError = TEXT("Введите имя корабля.");
		return false;
	}

	UShipBlueprintRegistry* Registry = nullptr;
	if (UGameInstance* GI = GetGameInstance())
	{
		Registry = GI->GetSubsystem<UShipBlueprintRegistry>();
	}

	const FName CandidateId = ShipBlueprintNaming::MakeShipIdFromDisplayName(DisplayName);
	FName SaveId = CandidateId;

	if (Registry && Registry->DoesPlayerBlueprintExist(CandidateId))
	{
		const FText Prompt = FText::Format(
			NSLOCTEXT(
				"SpaceshipCrew",
				"SaveAsOverwritePrompt",
				"Корабль с идентификатором «{0}» уже существует.\n\nДа — перезаписать.\nНет — сохранить как новый файл (с суффиксом)."),
			FText::FromName(CandidateId));

		const EAppReturnType::Type Choice = FMessageDialog::Open(EAppMsgType::YesNo, Prompt);
		if (Choice == EAppReturnType::Yes)
		{
			SaveId = CandidateId;
		}
		else
		{
			int32 Suffix = 1;
			while (Registry->DoesPlayerBlueprintExist(
				ShipBlueprintNaming::MakeUniquePlayerShipId(DisplayName, Suffix)))
			{
				++Suffix;
			}
			SaveId = ShipBlueprintNaming::MakeUniquePlayerShipId(DisplayName, Suffix);
		}
	}

	if (UShipBlueprintSessionSubsystem* Session = GetBlueprintSession())
	{
		SyncSessionFromDraft();
		return Session->SaveAs(SaveId, FText::FromString(DisplayName), OutError);
	}
	OutError = TEXT("Сессия недоступна.");
	return false;
}

bool ASpaceshipShipBuilderPlayerController::ConfirmDiscardDirtyAndContinue(TFunctionRef<void()> OnConfirmed)
{
	if (!IsShipSessionDirty())
	{
		OnConfirmed();
		return true;
	}

	const EAppReturnType::Type Result = FMessageDialog::Open(
		EAppMsgType::YesNo,
		NSLOCTEXT("SpaceshipCrew", "DiscardShipChanges", "Есть несохранённые изменения. Выйти без сохранения?"));
	if (Result == EAppReturnType::Yes)
	{
		OnConfirmed();
		return true;
	}
	return false;
}

void ASpaceshipShipBuilderPlayerController::RequestExitToMainMenu()
{
	ConfirmDiscardDirtyAndContinue([this]()
	{
		RequestExitToMainMenuForce();
	});
}

void ASpaceshipShipBuilderPlayerController::RequestExitToMainMenuForce()
{
	if (UWorld* World = GetWorld())
	{
		UGameplayStatics::OpenLevel(
			World,
			FName(SpaceshipCrewLevelTravel::GetPlayMapPackagePath()),
			false,
			SpaceshipCrewLevelTravel::GetMainMenuGameOptions());
	}
}

void ASpaceshipShipBuilderPlayerController::RefreshShipBuilderUi()
{
	if (ShipBuilderSlate.IsValid())
	{
		ShipBuilderSlate->Invalidate(EInvalidateWidgetReason::LayoutAndVolatility);
	}
}

void ASpaceshipShipBuilderPlayerController::EnsurePreviewActor()
{
	if (PreviewActor || !GetWorld())
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	PreviewActor = GetWorld()->SpawnActor<AShipBuilderModulePreviewActor>(
		AShipBuilderModulePreviewActor::StaticClass(),
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		SpawnParams);
}

void ASpaceshipShipBuilderPlayerController::RefreshPreviewFromDraft()
{
	EnsurePreviewActor();
	if (!PreviewActor)
	{
		return;
	}

	if (UShipModuleCatalog* Catalog = GetModuleCatalog())
	{
		PreviewActor->SetSelectedModuleInstanceId(SelectedModuleInstanceId);
		PreviewActor->SetHoveredModuleInstanceId(HoveredModuleInstanceId);
		PreviewActor->SetDragGhostTarget(bHasDragTargetCell, SelectedModuleInstanceId, DragTargetCell);
		PreviewActor->RebuildFromDraft(Draft, *Catalog);
		const FShipBuildValidationResult Validation = ComputeValidation();
		PreviewActor->SetPreviewDamageEnabled(!Validation.bIsValid);
	}
}

void ASpaceshipShipBuilderPlayerController::RotatePlacementLeft()
{
	const int32 SelectedIndex = FindDraftModuleIndexByInstanceId(SelectedModuleInstanceId);
	if (Draft.PlacedModules.IsValidIndex(SelectedIndex))
	{
		FShipBuilderDraftConfig::FPlacedModule& Selected = Draft.PlacedModules[SelectedIndex];
		Selected.YawStep = (Selected.YawStep + 3) % 4;
		NotifyDraftChanged();
		RefreshPreviewFromDraft();
		RefreshShipBuilderUi();
		return;
	}
	PendingPlacementYawStep = (PendingPlacementYawStep + 3) % 4;
	RefreshShipBuilderUi();
}

void ASpaceshipShipBuilderPlayerController::RotatePlacementRight()
{
	const int32 SelectedIndex = FindDraftModuleIndexByInstanceId(SelectedModuleInstanceId);
	if (Draft.PlacedModules.IsValidIndex(SelectedIndex))
	{
		FShipBuilderDraftConfig::FPlacedModule& Selected = Draft.PlacedModules[SelectedIndex];
		Selected.YawStep = (Selected.YawStep + 1) % 4;
		NotifyDraftChanged();
		RefreshPreviewFromDraft();
		RefreshShipBuilderUi();
		return;
	}
	PendingPlacementYawStep = (PendingPlacementYawStep + 1) % 4;
	RefreshShipBuilderUi();
}

void ASpaceshipShipBuilderPlayerController::MovePlacementUp()
{
	const int32 SelectedIndex = FindDraftModuleIndexByInstanceId(SelectedModuleInstanceId);
	if (Draft.PlacedModules.IsValidIndex(SelectedIndex))
	{
		++Draft.PlacedModules[SelectedIndex].GridPos.Z;
		RebuildConnectionsFromAdjacency();
		NotifyDraftChanged();
		RefreshPreviewFromDraft();
		RefreshShipBuilderUi();
		return;
	}
	++PendingPlacementZ;
	RefreshShipBuilderUi();
}

void ASpaceshipShipBuilderPlayerController::MovePlacementDown()
{
	const int32 SelectedIndex = FindDraftModuleIndexByInstanceId(SelectedModuleInstanceId);
	if (Draft.PlacedModules.IsValidIndex(SelectedIndex))
	{
		--Draft.PlacedModules[SelectedIndex].GridPos.Z;
		RebuildConnectionsFromAdjacency();
		NotifyDraftChanged();
		RefreshPreviewFromDraft();
		RefreshShipBuilderUi();
		return;
	}
	--PendingPlacementZ;
	RefreshShipBuilderUi();
}

void ASpaceshipShipBuilderPlayerController::OnRightMouseLookPressed()
{
	bRightMouseLookActive = true;
}

void ASpaceshipShipBuilderPlayerController::OnRightMouseLookReleased()
{
	bRightMouseLookActive = false;
}

void ASpaceshipShipBuilderPlayerController::UpdateRightMouseLook()
{
	if (!bRightMouseLookActive)
	{
		return;
	}
	float DeltaX = 0.0f;
	float DeltaY = 0.0f;
	GetInputMouseDelta(DeltaX, DeltaY);
	if (FMath::IsNearlyZero(DeltaX) && FMath::IsNearlyZero(DeltaY))
	{
		return;
	}
	constexpr float LookSensitivity = 0.12f;
	FRotator NewRotation = GetControlRotation();
	NewRotation.Yaw += DeltaX * LookSensitivity;
	NewRotation.Pitch = FMath::ClampAngle(NewRotation.Pitch - DeltaY * LookSensitivity, -85.0f, 85.0f);
	SetControlRotation(NewRotation);
}

void ASpaceshipShipBuilderPlayerController::OnSelectOrBeginDragPressed()
{
	bDraggingModule = TrySelectModuleUnderCursor();
	if (!bDraggingModule && !HoveredModuleInstanceId.IsNone())
	{
		SelectedModuleInstanceId = HoveredModuleInstanceId;
		bDraggingModule = true;
	}
	bDragMovementActivated = bDraggingModule;
	DraggedModuleInstanceId = bDraggingModule ? SelectedModuleInstanceId : NAME_None;
	float MouseX = 0.0f;
	float MouseY = 0.0f;
	if (GetMousePosition(MouseX, MouseY))
	{
		DragStartMousePos = FVector2D(MouseX, MouseY);
	}
	if (bDraggingModule)
	{
		const int32 ModuleIndex = FindDraftModuleIndexByInstanceId(DraggedModuleInstanceId);
		if (Draft.PlacedModules.IsValidIndex(ModuleIndex))
		{
			DragTargetCell = Draft.PlacedModules[ModuleIndex].GridPos;
			bHasDragTargetCell = true;
			RefreshPreviewFromDraft();
		}
	}
}

void ASpaceshipShipBuilderPlayerController::OnEndDragReleased()
{
	if (bDraggingModule && bHasDragTargetCell)
	{
		const int32 ModuleIndex = FindDraftModuleIndexByInstanceId(DraggedModuleInstanceId);
		if (Draft.PlacedModules.IsValidIndex(ModuleIndex)
			&& Draft.PlacedModules[ModuleIndex].GridPos != DragTargetCell
			&& !IsGridCellOccupied(DragTargetCell, DraggedModuleInstanceId))
		{
			Draft.PlacedModules[ModuleIndex].GridPos = DragTargetCell;
			RebuildConnectionsFromAdjacency();
			NotifyDraftChanged();
			RefreshPreviewFromDraft();
			RefreshShipBuilderUi();
		}
	}
	bDraggingModule = false;
	bDragMovementActivated = false;
	bHasDragTargetCell = false;
	DraggedModuleInstanceId = NAME_None;
	SelectedModuleInstanceId = NAME_None;
	RefreshPreviewFromDraft();
}

void ASpaceshipShipBuilderPlayerController::UpdateModuleDrag()
{
	if (!bDraggingModule || DraggedModuleInstanceId.IsNone())
	{
		return;
	}
	UShipModuleCatalog* Catalog = GetModuleCatalog();
	if (!Catalog)
	{
		return;
	}

	const int32 ModuleIndex = FindDraftModuleIndexByInstanceId(DraggedModuleInstanceId);
	if (!Draft.PlacedModules.IsValidIndex(ModuleIndex))
	{
		return;
	}

	FVector WorldOrigin = FVector::ZeroVector;
	FVector WorldDirection = FVector::ForwardVector;
	if (!DeprojectMousePositionToWorld(WorldOrigin, WorldDirection))
	{
		return;
	}

	const FIntVector& CurrentPos = Draft.PlacedModules[ModuleIndex].GridPos;
	const float DragPlaneZ = static_cast<float>(CurrentPos.Z) * SpaceshipShipBuilderInputPrivate::DragGridStepZ;
	const float Den = WorldDirection.Z;
	if (FMath::IsNearlyZero(Den))
	{
		return;
	}
	const float T = (DragPlaneZ - WorldOrigin.Z) / Den;
	if (T <= 0.0f)
	{
		return;
	}

	const FVector HitPoint = WorldOrigin + WorldDirection * T;
	const int32 NewGridX = FMath::RoundToInt(HitPoint.X / SpaceshipShipBuilderInputPrivate::DragGridStepXY);
	const int32 NewGridY = FMath::RoundToInt(HitPoint.Y / SpaceshipShipBuilderInputPrivate::DragGridStepXY);
	const FIntVector RawCell(NewGridX, NewGridY, Draft.PlacedModules[ModuleIndex].GridPos.Z);
	const FIntVector CandidateCell = FindBestSnappedCell(RawCell, DraggedModuleInstanceId);
	if (Draft.PlacedModules[ModuleIndex].GridPos == CandidateCell)
	{
		return;
	}
	if (IsGridCellOccupied(CandidateCell, DraggedModuleInstanceId))
	{
		return;
	}

	DragTargetCell = CandidateCell;
	bHasDragTargetCell = true;
	RefreshPreviewFromDraft();
}

void ASpaceshipShipBuilderPlayerController::UpdateHoveredModuleUnderCursor()
{
	const FName PreviousHover = HoveredModuleInstanceId;
	HoveredModuleInstanceId = NAME_None;

	if (Draft.PlacedModules.Num() == 0)
	{
		return;
	}
	UShipModuleCatalog* Catalog = GetModuleCatalog();
	if (!Catalog)
	{
		return;
	}

	float BestDistSq = TNumericLimits<float>::Max();
	for (const FShipBuilderDraftConfig::FPlacedModule& Placed : Draft.PlacedModules)
	{
		const UShipModuleDefinition* Def = Catalog->FindModuleById(Placed.ModuleId);
		if (!Def)
		{
			continue;
		}
		const FVector Center(
			static_cast<float>(Placed.GridPos.X) * SpaceshipShipBuilderInputPrivate::DragGridStepXY,
			static_cast<float>(Placed.GridPos.Y) * SpaceshipShipBuilderInputPrivate::DragGridStepXY,
			static_cast<float>(Placed.GridPos.Z) * SpaceshipShipBuilderInputPrivate::DragGridStepZ + Def->Size.Z * 0.5f);
		FVector2D ScreenPos;
		if (!ProjectWorldLocationToScreen(Center, ScreenPos))
		{
			continue;
		}
		float MouseX = 0.0f;
		float MouseY = 0.0f;
		if (!GetMousePosition(MouseX, MouseY))
		{
			continue;
		}
		const float DistSq = FVector2D::DistSquared(FVector2D(MouseX, MouseY), ScreenPos);
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			HoveredModuleInstanceId = Placed.InstanceId;
		}
	}

	if (BestDistSq > FMath::Square(SpaceshipShipBuilderInputPrivate::SelectionScreenRadiusPx))
	{
		HoveredModuleInstanceId = NAME_None;
	}
	if (PreviousHover != HoveredModuleInstanceId && !bDraggingModule)
	{
		RefreshPreviewFromDraft();
	}
}

bool ASpaceshipShipBuilderPlayerController::TrySelectModuleUnderCursor()
{
	const FName PreviousSelection = SelectedModuleInstanceId;
	SelectedModuleInstanceId = NAME_None;
	if (Draft.PlacedModules.Num() == 0)
	{
		return false;
	}
	UShipModuleCatalog* Catalog = GetModuleCatalog();
	if (!Catalog)
	{
		return false;
	}

	float BestDistSq = TNumericLimits<float>::Max();
	for (const FShipBuilderDraftConfig::FPlacedModule& Placed : Draft.PlacedModules)
	{
		const UShipModuleDefinition* Def = Catalog->FindModuleById(Placed.ModuleId);
		if (!Def)
		{
			continue;
		}
		const FVector Center(
			static_cast<float>(Placed.GridPos.X) * SpaceshipShipBuilderInputPrivate::DragGridStepXY,
			static_cast<float>(Placed.GridPos.Y) * SpaceshipShipBuilderInputPrivate::DragGridStepXY,
			static_cast<float>(Placed.GridPos.Z) * SpaceshipShipBuilderInputPrivate::DragGridStepZ + Def->Size.Z * 0.5f);
		FVector2D ScreenPos;
		if (!ProjectWorldLocationToScreen(Center, ScreenPos))
		{
			continue;
		}
		float MouseX = 0.0f;
		float MouseY = 0.0f;
		if (!GetMousePosition(MouseX, MouseY))
		{
			continue;
		}
		const float DistSq = FVector2D::DistSquared(FVector2D(MouseX, MouseY), ScreenPos);
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			SelectedModuleInstanceId = Placed.InstanceId;
		}
	}

	const bool bHasSelection = !SelectedModuleInstanceId.IsNone()
		&& BestDistSq <= FMath::Square(SpaceshipShipBuilderInputPrivate::SelectionScreenRadiusPx);
	if (!bHasSelection)
	{
		SelectedModuleInstanceId = NAME_None;
	}
	if (PreviousSelection != SelectedModuleInstanceId)
	{
		RefreshPreviewFromDraft();
	}
	return bHasSelection;
}

int32 ASpaceshipShipBuilderPlayerController::FindDraftModuleIndexByInstanceId(const FName InstanceId) const
{
	return Draft.PlacedModules.IndexOfByPredicate([InstanceId](const FShipBuilderDraftConfig::FPlacedModule& Placed)
	{
		return Placed.InstanceId == InstanceId;
	});
}

void ASpaceshipShipBuilderPlayerController::SyncLegacyModuleIds()
{
	Draft.ModuleIds.Reset(Draft.PlacedModules.Num());
	for (const FShipBuilderDraftConfig::FPlacedModule& Placed : Draft.PlacedModules)
	{
		Draft.ModuleIds.Add(Placed.ModuleId);
	}
}

FName ASpaceshipShipBuilderPlayerController::MakeNextDraftInstanceId() const
{
	return *FString::Printf(TEXT("Draft%d"), Draft.PlacedModules.Num());
}

void ASpaceshipShipBuilderPlayerController::RecomputeDraftConnectionSockets()
{
	auto FindPlacedByInstance = [this](const FName InstanceId) -> const FShipBuilderDraftConfig::FPlacedModule*
	{
		const int32 Index = FindDraftModuleIndexByInstanceId(InstanceId);
		return Draft.PlacedModules.IsValidIndex(Index) ? &Draft.PlacedModules[Index] : nullptr;
	};

	for (FShipBuilderDraftConfig::FConnection& Link : Draft.Connections)
	{
		const FShipBuilderDraftConfig::FPlacedModule* A = FindPlacedByInstance(Link.ModuleAInstanceId);
		const FShipBuilderDraftConfig::FPlacedModule* B = FindPlacedByInstance(Link.ModuleBInstanceId);
		if (!A || !B)
		{
			continue;
		}

		const bool bVertical = A->GridPos.Z != B->GridPos.Z;
		if (bVertical)
		{
			Link.ModuleASocketName = A->GridPos.Z > B->GridPos.Z ? FName(TEXT("Bottom")) : FName(TEXT("Top"));
			Link.ModuleBSocketName = A->GridPos.Z > B->GridPos.Z ? FName(TEXT("Top")) : FName(TEXT("Bottom"));
			continue;
		}

		const int32 DeltaX = A->GridPos.X - B->GridPos.X;
		const int32 DeltaY = A->GridPos.Y - B->GridPos.Y;
		if (FMath::Abs(DeltaY) > FMath::Abs(DeltaX))
		{
			Link.ModuleASocketName = DeltaY > 0 ? FName(TEXT("Left")) : FName(TEXT("Right"));
			Link.ModuleBSocketName = DeltaY > 0 ? FName(TEXT("Right")) : FName(TEXT("Left"));
		}
		else
		{
			Link.ModuleASocketName = DeltaX > 0 ? FName(TEXT("Back")) : FName(TEXT("Front"));
			Link.ModuleBSocketName = DeltaX > 0 ? FName(TEXT("Front")) : FName(TEXT("Back"));
		}
	}
}

void ASpaceshipShipBuilderPlayerController::RebuildConnectionsFromAdjacency()
{
	const UShipModuleCatalog* Catalog = GetModuleCatalog();
	Draft.Connections.Reset();
	TSet<FString> SeenPairs;
	for (int32 i = 0; i < Draft.PlacedModules.Num(); ++i)
	{
		for (int32 j = i + 1; j < Draft.PlacedModules.Num(); ++j)
		{
			const FShipBuilderDraftConfig::FPlacedModule& A = Draft.PlacedModules[i];
			const FShipBuilderDraftConfig::FPlacedModule& B = Draft.PlacedModules[j];
			const FIntVector D = B.GridPos - A.GridPos;
			const UShipModuleDefinition* DefA = Catalog ? Catalog->FindModuleById(A.ModuleId) : nullptr;
			const UShipModuleDefinition* DefB = Catalog ? Catalog->FindModuleById(B.ModuleId) : nullptr;
			const float AX = DefA ? DefA->Size.X : 400.0f;
			const float AY = DefA ? DefA->Size.Y : 400.0f;
			const float AZ = DefA ? DefA->Size.Z : 300.0f;
			const float BX = DefB ? DefB->Size.X : 400.0f;
			const float BY = DefB ? DefB->Size.Y : 400.0f;
			const float BZ = DefB ? DefB->Size.Z : 300.0f;
			const int32 NeedStepX = FMath::Max(1, FMath::RoundToInt((AX * 0.5f + BX * 0.5f) / SpaceshipShipBuilderInputPrivate::DragGridStepXY));
			const int32 NeedStepY = FMath::Max(1, FMath::RoundToInt((AY * 0.5f + BY * 0.5f) / SpaceshipShipBuilderInputPrivate::DragGridStepXY));
			const int32 NeedStepZ = FMath::Max(1, FMath::RoundToInt((AZ * 0.5f + BZ * 0.5f) / SpaceshipShipBuilderInputPrivate::DragGridStepZ));

			const bool bAdjacentX = FMath::Abs(D.X) == NeedStepX && D.Y == 0 && D.Z == 0;
			const bool bAdjacentY = FMath::Abs(D.Y) == NeedStepY && D.X == 0 && D.Z == 0;
			const bool bAdjacentZ = FMath::Abs(D.Z) == NeedStepZ && D.X == 0 && D.Y == 0;
			if (!bAdjacentX && !bAdjacentY && !bAdjacentZ)
			{
				continue;
			}

			FShipBuilderDraftConfig::FConnection Link;
			Link.ModuleAInstanceId = A.InstanceId;
			Link.ModuleBInstanceId = B.InstanceId;

			if (bAdjacentZ)
			{
				Link.ModuleASocketName = D.Z > 0 ? FName(TEXT("Top")) : FName(TEXT("Bottom"));
				Link.ModuleBSocketName = D.Z > 0 ? FName(TEXT("Bottom")) : FName(TEXT("Top"));
			}
			else if (bAdjacentY)
			{
				Link.ModuleASocketName = D.Y > 0 ? FName(TEXT("Right")) : FName(TEXT("Left"));
				Link.ModuleBSocketName = D.Y > 0 ? FName(TEXT("Left")) : FName(TEXT("Right"));
			}
			else
			{
				Link.ModuleASocketName = D.X > 0 ? FName(TEXT("Front")) : FName(TEXT("Back"));
				Link.ModuleBSocketName = D.X > 0 ? FName(TEXT("Back")) : FName(TEXT("Front"));
			}

			const FString PairKey = FString::Printf(TEXT("%s|%s|%s|%s"),
				*A.InstanceId.ToString(),
				*B.InstanceId.ToString(),
				*Link.ModuleASocketName.ToString(),
				*Link.ModuleBSocketName.ToString());
			if (!SeenPairs.Contains(PairKey))
			{
				SeenPairs.Add(PairKey);
				Draft.Connections.Add(Link);
			}
		}
	}
}

bool ASpaceshipShipBuilderPlayerController::IsGridCellOccupied(const FIntVector& Cell, const FName IgnoreInstanceId) const
{
	for (const FShipBuilderDraftConfig::FPlacedModule& Placed : Draft.PlacedModules)
	{
		if (!IgnoreInstanceId.IsNone() && Placed.InstanceId == IgnoreInstanceId)
		{
			continue;
		}
		if (Placed.GridPos == Cell)
		{
			return true;
		}
	}
	return false;
}

FIntVector ASpaceshipShipBuilderPlayerController::FindBestSnappedCell(const FIntVector& RawCell, const FName MovingInstanceId) const
{
	// No adjacency restriction: place directly to hovered grid cell.
	if (!IsGridCellOccupied(RawCell, MovingInstanceId))
	{
		return RawCell;
	}

	// If occupied, find nearest free cell around raw target.
	for (int32 Radius = 1; Radius <= 8; ++Radius)
	{
		for (int32 dx = -Radius; dx <= Radius; ++dx)
		{
			for (int32 dy = -Radius; dy <= Radius; ++dy)
			{
				const FIntVector Candidate(RawCell.X + dx, RawCell.Y + dy, RawCell.Z);
				if (!IsGridCellOccupied(Candidate, MovingInstanceId))
				{
					return Candidate;
				}
			}
		}
	}

	return RawCell;
}

void ASpaceshipShipBuilderPlayerController::ToggleCatalog()
{
	bCatalogOpen = !bCatalogOpen;
	if (bCatalogOpen)
	{
		bChecklistOpen = false;
		EnsureCatalogCategoryIndexValid();
		if (ShipBuilderSlate.IsValid())
		{
			ShipBuilderSlate->RebuildCatalogPanel();
		}
	}
	RefreshShipBuilderUi();
}

void ASpaceshipShipBuilderPlayerController::ToggleChecklist()
{
	bChecklistOpen = !bChecklistOpen;
	if (bChecklistOpen)
	{
		bCatalogOpen = false;
	}
	RefreshShipBuilderUi();
}

void ASpaceshipShipBuilderPlayerController::OnExitPressed()
{
	RequestExitToMainMenu();
}
