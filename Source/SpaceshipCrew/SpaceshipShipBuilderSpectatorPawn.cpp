#include "SpaceshipShipBuilderSpectatorPawn.h"

#include "GameFramework/PawnMovementComponent.h"

ASpaceshipShipBuilderSpectatorPawn::ASpaceshipShipBuilderSpectatorPawn()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ASpaceshipShipBuilderSpectatorPawn::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if (UPawnMovementComponent* Move = GetMovementComponent())
	{
		PrimaryActorTick.AddPrerequisite(Move, Move->PrimaryComponentTick);
	}
}

void ASpaceshipShipBuilderSpectatorPawn::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	ClampToFloorHeight();
}

void ASpaceshipShipBuilderSpectatorPawn::ClampToFloorHeight()
{
	FVector Loc = GetActorLocation();
	if (Loc.Z >= MinWorldZ)
	{
		return;
	}
	Loc.Z = MinWorldZ;
	SetActorLocation(Loc, false, nullptr, ETeleportType::TeleportPhysics);
}
