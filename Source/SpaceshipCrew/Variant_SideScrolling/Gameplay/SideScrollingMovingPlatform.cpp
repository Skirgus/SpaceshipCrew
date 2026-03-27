// Copyright Epic Games, Inc. All Rights Reserved.


#include "SideScrollingMovingPlatform.h"
#include "Components/SceneComponent.h"

ASideScrollingMovingPlatform::ASideScrollingMovingPlatform()
{
	PrimaryActorTick.bCanEverTick = false;

	// создать root comp
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void ASideScrollingMovingPlatform::Interaction(AActor* Interactor)
{
	// игнорировать interactions если мы already движущаяся
	if (bMoving)
	{
		return;
	}

	// поднять движение flag
	bMoving = true;

	// передать control для BP для actual movement
	BP_MoveToTarget();
}

void ASideScrollingMovingPlatform::ResetInteraction()
{
	// игнорировать если этот is a one-shot платформа
	if (bOneShot)
	{
		return;
	}

	// сбросить движение flag
	bMoving = false;
}




