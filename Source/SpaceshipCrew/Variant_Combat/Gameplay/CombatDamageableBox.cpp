// Copyright Epic Games, Inc. All Rights Reserved.


#include "CombatDamageableBox.h"
#include "Components/StaticMeshComponent.h"
#include "TimerManager.h"
#include "Engine/World.h"

ACombatDamageableBox::ACombatDamageableBox()
{
	PrimaryActorTick.bCanEverTick = false;

	// создать mesh
	RootComponent = Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));

	// установить коллизия properties
	Mesh->SetCollisionProfileName(FName("BlockAllDynamic"));

	// включить физика
	Mesh->SetSimulatePhysics(true);

	// отключить navigation relevance so boxes don't влияют на NavMesh generation
	Mesh->bNavigationRelevant = false;
}

void ACombatDamageableBox::RemoveFromLevel()
{
	// уничтожить этот актор
	Destroy();
}

void ACombatDamageableBox::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// очистить death таймер
	GetWorld()->GetTimerManager().ClearTimer(DeathTimer);
}

void ACombatDamageableBox::ApplyDamage(float Damage, AActor* DamageCauser, const FVector& DamageLocation, const FVector& DamageImpulse)
{
	// только process урон если we still имеет HP
	if (CurrentHP > 0.0f)
	{
 // apply урон
		CurrentHP -= Damage;

 // are we dead?
		if (CurrentHP <= 0.0f)
		{
			HandleDeath();
		}

 // apply a физика impulse для box, ignoring its mass
		Mesh->AddImpulseAtLocation(DamageImpulse * Mesh->GetMass(), DamageLocation);

 // вызвать BP handler для play effects, etc.
		OnBoxDamaged(DamageLocation, DamageImpulse);
	}
}

void ACombatDamageableBox::HandleDeath()
{
	// change коллизия объект type для Visibility so we игнорировать most interactions but still retain физика коллизияs
	Mesh->SetCollisionObjectType(ECC_Visibility);

	// вызвать BP handler для play effects, etc.
	OnBoxDestroyed();

	// установить up death Очистка таймер
	GetWorld()->GetTimerManager().SetTimer(DeathTimer, this, &ACombatDamageableBox::RemoveFromLevel, DeathDelayTime);
}

void ACombatDamageableBox::ApplyHealing(float Healing, AActor* Healer)
{
	// заглушка
}

void ACombatDamageableBox::NotifyDanger(const FVector& DangerLocation, AActor* DangerSource)
{
	// заглушка
}





