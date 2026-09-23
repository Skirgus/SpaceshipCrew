#include "CrewWorkstation.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"

ACrewWorkstation::ACrewWorkstation()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(MeshComponent);
	MeshComponent->SetCollisionProfileName(TEXT("BlockAllDynamic"));

	OccupyAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("OccupyAnchor"));
	OccupyAnchor->SetupAttachment(RootComponent);
	OccupyAnchor->SetRelativeLocation(FVector(-80.0f, 0.0f, 0.0f));
}

bool ACrewWorkstation::CanInteract_Implementation(APawn* InstigatorPawn) const
{
	if (!InstigatorPawn)
	{
		return false;
	}
	if (OperatorPawn == nullptr)
	{
		return true;
	}
	return OperatorPawn == InstigatorPawn;
}

FText ACrewWorkstation::GetInteractPrompt_Implementation(APawn* InstigatorPawn) const
{
	if (OperatorPawn == InstigatorPawn)
	{
		return NSLOCTEXT("SpaceshipCrew", "Station_Leave", "Освободить станцию");
	}
	return NSLOCTEXT("SpaceshipCrew", "Station_Occupy", "Занять станцию");
}

void ACrewWorkstation::Interact_Implementation(APawn* InstigatorPawn)
{
	if (!InstigatorPawn)
	{
		return;
	}
	if (OperatorPawn == InstigatorPawn)
	{
		Leave();
		return;
	}
	Occupy(InstigatorPawn);
}

bool ACrewWorkstation::Occupy(APawn* Operator)
{
	if (!Operator || OperatorPawn)
	{
		return false;
	}

	OperatorPawn = Operator;
	Operator->SetActorLocation(OccupyAnchor->GetComponentLocation());
	Operator->SetActorRotation(OccupyAnchor->GetComponentRotation());

	if (ACharacter* AsCharacter = Cast<ACharacter>(Operator))
	{
		if (UCharacterMovementComponent* Move = AsCharacter->GetCharacterMovement())
		{
			Move->DisableMovement();
		}
	}

	OnOccupied.Broadcast(Operator);
	return true;
}

void ACrewWorkstation::Leave()
{
	if (!OperatorPawn)
	{
		return;
	}

	if (ACharacter* AsCharacter = Cast<ACharacter>(OperatorPawn))
	{
		if (UCharacterMovementComponent* Move = AsCharacter->GetCharacterMovement())
		{
			Move->SetMovementMode(MOVE_Walking);
		}
	}

	OperatorPawn = nullptr;
	OnLeft.Broadcast();
}

void ACrewWorkstation::SetDisplayMesh(UStaticMesh* InMesh)
{
	if (MeshComponent && InMesh)
	{
		MeshComponent->SetStaticMesh(InMesh);
	}
}
