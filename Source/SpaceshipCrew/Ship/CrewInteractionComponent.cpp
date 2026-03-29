// Copyright Epic Games, Inc. All Rights Reserved.

#include "CrewInteractionComponent.h"
#include "CrewRoleComponent.h"
#include "ShipCrewCharacter.h"
#include "ShipInteractableBase.h"
#include "SpaceshipCrew.h"
#include "Components/PrimitiveComponent.h"
#include "CollisionShape.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Math/Box.h"

namespace
{
AShipInteractableBase* ResolveInteractableFromHit(const FHitResult& Hit)
{
	if (AActor* A = Hit.GetActor())
	{
		if (AShipInteractableBase* I = Cast<AShipInteractableBase>(A))
		{
			return I;
		}
	}
	// Child Actor / вложенные компоненты: владелец примитива или цепочка AttachParent.
	if (UPrimitiveComponent* Prim = Hit.GetComponent())
	{
		AActor* Walk = Prim->GetOwner();
		for (int32 i = 0; i < 6 && Walk; ++i)
		{
			if (AShipInteractableBase* I = Cast<AShipInteractableBase>(Walk))
			{
				return I;
			}
			Walk = Walk->GetAttachParentActor();
		}
	}
	return nullptr;
}
} // namespace

UCrewInteractionComponent::UCrewInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UCrewInteractionComponent::RequestInteract()
{
	APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn || !Pawn->IsLocallyControlled())
	{
		return;
	}
	AShipInteractableBase* Station = Cast<AShipInteractableBase>(TraceInteractable());
	if (EvaluateInteractAvailability(Station) != ECrewInteractAvailability::Ready)
	{
		return;
	}
	ServerTryInteract(Station);
}

AActor* UCrewInteractionComponent::GetFocusedInteractable() const
{
	return TraceInteractable();
}

ECrewInteractAvailability UCrewInteractionComponent::GetInteractAvailability() const
{
	AShipInteractableBase* Station = Cast<AShipInteractableBase>(TraceInteractable());
	return EvaluateInteractAvailability(Station);
}

ECrewInteractAvailability UCrewInteractionComponent::EvaluateInteractAvailability(AShipInteractableBase* Station) const
{
	if (!Station)
	{
		return ECrewInteractAvailability::None;
	}
	const AShipCrewCharacter* Crew = Cast<AShipCrewCharacter>(GetOwner());
	if (!Crew)
	{
		return ECrewInteractAvailability::None;
	}
	if (!Crew->InteractAction)
	{
		return ECrewInteractAvailability::NeedBindInteractAction;
	}
	if (!ValidateInteractDistance(Station))
	{
		return ECrewInteractAvailability::TooFar;
	}
	// Как UShipSystemsComponent::InternalApplyAction: нужен CrewRole и RoleDefinition, иначе CanUseStation ложен.
	if (!Crew->CrewRole)
	{
		return ECrewInteractAvailability::NoCrewRole;
	}
	if (!Crew->CrewRole->RoleDefinition)
	{
		return ECrewInteractAvailability::NoCrewRole;
	}
	if (!Crew->CrewRole->CanUseStation(Station->RequiredPermission))
	{
		return ECrewInteractAvailability::NoPermission;
	}
	return ECrewInteractAvailability::Ready;
}

AActor* UCrewInteractionComponent::TraceInteractable() const
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn)
	{
		return nullptr;
	}
	AController* Controller = Pawn->GetController();
	if (!Controller)
	{
		return nullptr;
	}

	// Луч из камеры через позицию курсора на экране — подсказка и hit совпадают с тем, куда смотрит мышь.
	// Fallback: GetPlayerViewPoint (центр вида), если deproject не удался.
	FVector TraceStart;
	FVector TraceEnd;
	const APlayerController* PC = Cast<APlayerController>(Controller);
	FVector RayDir;
	if (PC && PC->DeprojectMousePositionToWorld(TraceStart, RayDir))
	{
		RayDir = RayDir.GetSafeNormal();
		TraceEnd = TraceStart + RayDir * TraceDistance;
	}
	else
	{
		FVector ViewLoc;
		FRotator ViewRot;
		Controller->GetPlayerViewPoint(ViewLoc, ViewRot);
		TraceStart = ViewLoc;
		TraceEnd = ViewLoc + ViewRot.Vector() * TraceDistance;
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(CrewInteractTrace), false, Pawn);
	TArray<FHitResult> Hits;
	const float R = FMath::Max(0.f, TraceSweepRadius);
	bool bAnyHit = false;
	if (R > KINDA_SMALL_NUMBER)
	{
		const FCollisionShape SweepShape = FCollisionShape::MakeSphere(R);
		bAnyHit = GetWorld()->SweepMultiByChannel(
			Hits,
			TraceStart,
			TraceEnd,
			FQuat::Identity,
			ECC_Visibility,
			SweepShape,
			Params);
	}
	else
	{
		bAnyHit = GetWorld()->LineTraceMultiByChannel(Hits, TraceStart, TraceEnd, ECC_Visibility, Params);
	}
	if (!bAnyHit)
	{
		return nullptr;
	}
	// Ближайший по лучу interactable (пол, корпус и т.д. могут давать несколько hit подряд).
	AShipInteractableBase* Best = nullptr;
	float BestTime = TNumericLimits<float>::Max();
	for (const FHitResult& Hit : Hits)
	{
		if (AShipInteractableBase* I = ResolveInteractableFromHit(Hit))
		{
			const float T = Hit.Time;
			if (T < BestTime)
			{
				BestTime = T;
				Best = I;
			}
		}
	}
	return Best;
}

bool UCrewInteractionComponent::ValidateInteractDistance(AActor* Target) const
{
	if (!Target || !GetOwner())
	{
		return false;
	}
	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn)
	{
		return false;
	}
	const FVector PawnLoc = Pawn->GetActorLocation();
	// Раньше: центр пешки ↔ центр актора. Камера (SpringArm) «видит» станцию, а root станции/корабля
	// может быть дальше TraceDistance*1.5 от капсулы — сервер отклонял RPC, хотя луч клиента попадал.
	FVector Origin, Extent;
	Target->GetActorBounds(false, Origin, Extent);
	const FBox Box(Origin - Extent, Origin + Extent);
	const FVector Closest = Box.GetClosestPointTo(PawnLoc);
	const float DistSq = FVector::DistSquared(PawnLoc, Closest);
	const float MaxReach = TraceDistance + 500.f;
	return DistSq <= FMath::Square(MaxReach);
}

bool UCrewInteractionComponent::ServerTryInteract_Validate(AActor* TargetActor)
{
	return TargetActor != nullptr;
}

void UCrewInteractionComponent::ServerTryInteract_Implementation(AActor* TargetActor)
{
	if (!TargetActor || !GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	if (!ValidateInteractDistance(TargetActor))
	{
		UE_LOG(LogSpaceshipCrew, Warning, TEXT("ServerTryInteract: слишком далеко от станции (пешка %s, цель %s)."),
			*GetNameSafe(GetOwner()), *GetNameSafe(TargetActor));
		return;
	}
	AShipInteractableBase* Interactable = Cast<AShipInteractableBase>(TargetActor);
	if (!Interactable)
	{
		return;
	}
	APawn* Pawn = Cast<APawn>(GetOwner());
	AController* C = Pawn ? Pawn->GetController() : nullptr;
	Interactable->ExecuteInteract(C, Pawn);
}
