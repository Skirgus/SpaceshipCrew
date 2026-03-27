// Copyright Epic Games, Inc. All Rights Reserved.


#include "CombatActivationVolume.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"
#include "CombatActivatable.h"

ACombatActivationVolume::ACombatActivationVolume()
{
	PrimaryActorTick.bCanEverTick = false;

	// создать box volume
	RootComponent = Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	check(Box);

	// установить box's extent
	Box->SetBoxExtent(FVector(500.0f, 500.0f, 500.0f));

	// установить по умолчанию коллизия profile для пересечение all dynamic
	Box->SetCollisionProfileName(FName("OverlapAllDynamic"));

	// привязать begin пересечение
	Box->OnComponentBeginOverlap.AddDynamic(this, &ACombatActivationVolume::OnOverlap);
}

void ACombatActivationVolume::OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// has a Персонаж входе volume?
	ACharacter* PlayerCharacter = Cast<ACharacter>(OtherActor);

	if (PlayerCharacter)
	{
 // is Персонаж controlled через a игрок
		if (PlayerCharacter->IsPlayerControlled())
		{
 // process акторы для activate список
			for (AActor* CurrentActor : ActorsToActivate)
			{
 // is referenced актор activatable?
				if(ICombatActivatable* Activatable = Cast<ICombatActivatable>(CurrentActor))
				{
					Activatable->ActivateInteraction(PlayerCharacter);
				}
			}
		}
	}

}



