// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShipPlayerController.h"
#include "ShipStatusWidget.h"
#include "TimerManager.h"

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
