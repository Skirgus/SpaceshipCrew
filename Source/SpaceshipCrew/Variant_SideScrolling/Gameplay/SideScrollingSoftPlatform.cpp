// Copyright Epic Games, Inc. All Rights Reserved.


#include "SideScrollingSoftPlatform.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "SideScrollingCharacter.h"

ASideScrollingSoftPlatform::ASideScrollingSoftPlatform()
{
 	PrimaryActorTick.bCanEverTick = true;

	// создать Корневой компонент
	RootComponent = Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// создать mesh
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);

	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Mesh->SetCollisionObjectType(ECC_WorldStatic);
	Mesh->SetCollisionResponseToAllChannels(ECR_Block);

	// создать коллизия проверить box
	CollisionCheckBox = CreateDefaultSubobject<UBoxComponent>(TEXT("Collision Check Box"));
	CollisionCheckBox->SetupAttachment(Mesh);

	CollisionCheckBox->SetRelativeLocation(FVector(0.0f, 0.0f, -40.0f));
	CollisionCheckBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionCheckBox->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionCheckBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionCheckBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	// подписаться для пересечение события
	CollisionCheckBox->OnComponentBeginOverlap.AddDynamic(this, &ASideScrollingSoftPlatform::OnSoftCollisionOverlap);
}

void ASideScrollingSoftPlatform::OnSoftCollisionOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// имеет we пересечениеped a персонаж?
	if (ASideScrollingCharacter* Char = Cast<ASideScrollingCharacter>(OtherActor))
	{
 // отключить soft коллизия channel
		Char->SetSoftCollision(true);
	}
}

void ASideScrollingSoftPlatform::NotifyActorEndOverlap(AActor* OtherActor)
{
	Super::NotifyActorEndOverlap(OtherActor);

	// имеет we пересечениеped a персонаж?
	if (ASideScrollingCharacter* Char = Cast<ASideScrollingCharacter>(OtherActor))
	{
 // включить soft коллизия channel
		Char->SetSoftCollision(false);
	}
}




