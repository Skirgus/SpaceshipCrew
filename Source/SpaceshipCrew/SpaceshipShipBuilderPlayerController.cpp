#include "SpaceshipShipBuilderPlayerController.h"

#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Kismet/GameplayStatics.h"
#include "ShipBuilderDomainGlue.h"
#include "ShipModuleCatalog.h"
#include "ShipModuleDefinition.h"
#include "ShipModuleTypes.h"
#include "UI/SSpaceshipShipBuilderRoot.h"
#include "Menu/SpaceshipCrewLevelTravel.h"

void ASpaceshipShipBuilderPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalPlayerController())
	{
		return;
	}

	bShowMouseCursor = true;
	FInputModeGameAndUI Mode;
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(Mode);

	if (UWorld* World = GetWorld())
	{
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
	if (!ModuleId.IsNone())
	{
		Draft.ModuleIds.Add(ModuleId);
	}
}

void ASpaceshipShipBuilderPlayerController::RequestExitToMainMenu()
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
