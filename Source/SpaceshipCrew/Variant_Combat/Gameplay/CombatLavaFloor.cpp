// Copyright Epic Games, Inc. All Rights Reserved.


#include "CombatLavaFloor.h"
#include "CombatDamageable.h"
#include "Components/StaticMeshComponent.h"

ACombatLavaFloor::ACombatLavaFloor()
{
	PrimaryActorTick.bCanEverTick = false;

	// создать mesh
	RootComponent = Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));

	// привязать попадание handler
	Mesh->OnComponentHit.AddDynamic(this, &ACombatLavaFloor::OnFloorHit);
}

void ACombatLavaFloor::OnFloorHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// проверить если попадание актор is уронable через casting для интерфейс
	if (ICombatDamageable* Damageable = Cast<ICombatDamageable>(OtherActor))
	{
 // урон актор
		Damageable->ApplyDamage(Damage, this, Hit.ImpactPoint, FVector::ZeroVector);
	}
}




