#include "SpaceshipShipBuilderPlayerController.h"

#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Kismet/GameplayStatics.h"
#include "ShipBuilderDomainGlue.h"
#include "ShipModuleCatalog.h"
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
	HoveredCatalogModuleId = ModuleId;
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
		ShipBuilderSlate->RequestRefresh();
	}
}

void ASpaceshipShipBuilderPlayerController::ToggleCatalog()
{
	bCatalogOpen = !bCatalogOpen;
	if (bCatalogOpen)
	{
		bChecklistOpen = false;
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
