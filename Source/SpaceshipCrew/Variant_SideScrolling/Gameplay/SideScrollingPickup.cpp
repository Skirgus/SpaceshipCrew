// Copyright Epic Games, Inc. All Rights Reserved.


#include "SideScrollingPickup.h"
#include "GameFramework/Character.h"
#include "SideScrollingGameMode.h"
#include "Components/SphereComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"

ASideScrollingPickup::ASideScrollingPickup()
{
	PrimaryActorTick.bCanEverTick = false;

	// создать root comp
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// создать bounding sphere
	Sphere = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Sphere->SetupAttachment(RootComponent);

	Sphere->SetSphereRadius(100.0f);

	Sphere->SetCollisionObjectType(ECC_WorldDynamic);
	Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Sphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	Sphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	// add пересечение handler
	OnActorBeginOverlap.AddDynamic(this, &ASideScrollingPickup::BeginOverlap);
}

void ASideScrollingPickup::BeginOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	// имеет we collided against a персонаж?
	if (ACharacter* OverlappedCharacter = Cast<ACharacter>(OtherActor))
	{
 // is этот игрок персонаж?
		if (OverlappedCharacter->IsPlayerControlled())
		{
 // получить game mode
			if (ASideScrollingGameMode* GM = Cast<ASideScrollingGameMode>(GetWorld()->GetAuthGameMode()))
			{
 // tell game mode для process a подбор
				GM->ProcessPickup();

 // отключить коллизия so we don't получить picked up again
				SetActorEnableCollision(false);

 // вызвать BP handler. It will be responsible для destroying подбор
				BP_OnPickedUp();
			}
		}
	}
}



