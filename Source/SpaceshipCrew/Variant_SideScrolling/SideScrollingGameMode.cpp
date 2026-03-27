// Copyright Epic Games, Inc. All Rights Reserved.


#include "SideScrollingGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"
#include "SideScrollingUI.h"
#include "SideScrollingPickup.h"

void ASideScrollingGameMode::BeginPlay()
{
	Super::BeginPlay();

	// создать game UI
	APlayerController* OwningPlayer = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	
	UserInterface = CreateWidget<USideScrollingUI>(OwningPlayer, UserInterfaceClass);

	check(UserInterface);
}

void ASideScrollingGameMode::ProcessPickup()
{
	// increment подборs counter
	++PickupsCollected;

	// если этот is первый подбор we collect, show UI
	if (PickupsCollected == 1)
	{
		UserInterface->AddToViewport(0);
	}

	// обновить подборs counter на UI
	UserInterface->UpdatePickups(PickupsCollected);
}



