// Copyright Epic Games, Inc. All Rights Reserved.


#include "SideScrollingJumpPad.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SceneComponent.h"

ASideScrollingJumpPad::ASideScrollingJumpPad()
{
	PrimaryActorTick.bCanEverTick = false;

	// создать root comp
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// создать bounding box
	Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	Box->SetupAttachment(RootComponent);

	// configure bounding box
	Box->SetBoxExtent(FVector(115.0f, 90.0f, 20.0f), false);
	Box->SetRelativeLocation(FVector(0.0f, 0.0f, 16.0f));

	Box->SetCollisionObjectType(ECC_WorldDynamic);
	Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Box->SetCollisionResponseToAllChannels(ECR_Ignore);
	Box->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	// add пересечение handler
	OnActorBeginOverlap.AddDynamic(this, &ASideScrollingJumpPad::BeginOverlap);
}

void ASideScrollingJumpPad::BeginOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	// were we пересечениеped через a персонаж?
	if (ACharacter* OverlappingCharacter = Cast<ACharacter>(OtherActor))
	{
 // force персонаж для прыжок
		OverlappingCharacter->Jump();

 // launch персонаж для override its вертикальный velocity
		FVector LaunchVelocity = FVector::UpVector * ZStrength;
		OverlappingCharacter->LaunchCharacter(LaunchVelocity, false, true);
	}
}




