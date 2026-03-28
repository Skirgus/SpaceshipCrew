// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShipPlayerController.h"
#include "ShipStatusWidget.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

void AShipPlayerController::ApplyShipViewAndInputDefaults()
{
	if (!IsLocalPlayerController())
	{
		return;
	}

	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
	bShowMouseCursor = false;
	bEnableClickEvents = false;
	bEnableMouseOverEvents = false;
	SetIgnoreLookInput(false);
	SetIgnoreMoveInput(false);
}

void AShipPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// Пешка игрока создаётся в AShipGameMode::EnsureCrewSpawned до Possess; без явного ViewTarget
	// камера иногда остаётся на дефолтной точке (например у PlayerStart), хотя персонаж уже ходит.
	if (InPawn && IsLocalPlayerController())
	{
		SetViewTarget(InPawn);
		ApplyShipViewAndInputDefaults();
	}
}

void AShipPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	if (ShipStatusWidgetClass)
	{
		ShipStatusWidget = CreateWidget<UShipStatusWidget>(this, ShipStatusWidgetClass);
		if (ShipStatusWidget)
		{
			ShipStatusWidget->AddToPlayerScreen(100);
		}
	}

	ApplyShipViewAndInputDefaults();

	if (GetWorld())
	{
		GetWorldTimerManager().SetTimer(
			HudRefreshTimer,
			this,
			&AShipPlayerController::RefreshShipHud,
			FMath::Max(0.05f, HudRefreshInterval),
			true
		);
		RefreshShipHud();
	}
}

void AShipPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(HudRefreshTimer);
	}
	Super::EndPlay(EndPlayReason);
}

void AShipPlayerController::RefreshShipHud()
{
	if (ShipStatusWidget)
	{
		ShipStatusWidget->RefreshFromController(this);
	}
}
