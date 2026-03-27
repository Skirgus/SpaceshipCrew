// Copyright Epic Games, Inc. All Rights Reserved.


#include "CombatCheckpointVolume.h"
#include "CombatCharacter.h"
#include "CombatPlayerController.h"

ACombatCheckpointVolume::ACombatCheckpointVolume()
{
	// создать box volume
	RootComponent = Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	check(Box);

	// установить box's extent
	Box->SetBoxExtent(FVector(500.0f, 500.0f, 500.0f));

	// установить по умолчанию коллизия profile для пересечение all dynamic
	Box->SetCollisionProfileName(FName("OverlapAllDynamic"));

	// привязать begin пересечение
	Box->OnComponentBeginOverlap.AddDynamic(this, &ACombatCheckpointVolume::OnOverlap);
}

void ACombatCheckpointVolume::OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// убедиться we использовать этот только once
	if (bCheckpointUsed)
	{
		return;
	}
		
	// has игрок входе этот volume?
	ACombatCharacter* PlayerCharacter = Cast<ACombatCharacter>(OtherActor);

	if (PlayerCharacter)
	{
		if (ACombatPlayerController* PC = Cast<ACombatPlayerController>(PlayerCharacter->GetController()))
		{
 // поднять проверитьpoint used flag
			bCheckpointUsed = true;

 // обновить игрок's респавн проверитьpoint
			PC->SetRespawnTransform(PlayerCharacter->GetActorTransform());
		}

	}
}




