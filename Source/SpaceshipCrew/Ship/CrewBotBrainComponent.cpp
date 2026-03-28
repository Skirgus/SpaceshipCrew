// Copyright Epic Games, Inc. All Rights Reserved.

#include "CrewBotBrainComponent.h"
#include "CrewAIController.h"
#include "ShipActor.h"
#include "ShipInteractableBase.h"
#include "ShipSystemsComponent.h"
#include "GameFramework/Pawn.h"
#include "AIController.h"
#include "EngineUtils.h"

UCrewBotBrainComponent::UCrewBotBrainComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCrewBotBrainComponent::RefreshBrainTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ThinkTimer);
		if (bBrainEnabled && GetOwner() && GetOwner()->HasAuthority())
		{
			World->GetTimerManager().SetTimer(ThinkTimer, this, &UCrewBotBrainComponent::Think, ThinkInterval, true);
		}
	}
}

void UCrewBotBrainComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ThinkTimer);
	}
	Super::EndPlay(EndPlayReason);
}

AShipInteractableBase* UCrewBotBrainComponent::FindBestInteractable() const
{
	UShipSystemsComponent* Sys = nullptr;
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AShipActor> It(World); It; ++It)
		{
			Sys = It->GetShipSystems();
			if (Sys)
			{
				break;
			}
		}
	}
	if (!Sys)
	{
		return nullptr;
	}

	FName DesiredAction = NAME_None;
	if (Sys->FireIntensity > 0.15f)
	{
		DesiredAction = FName(TEXT("FightFire"));
	}
	else if (Sys->HullIntegrity < 0.45f)
	{
		DesiredAction = FName(TEXT("RepairHull"));
	}
	else if (Sys->OxygenLevel < 0.35f)
	{
		DesiredAction = FName(TEXT("AdjustReactor"));
	}
	else
	{
		DesiredAction = FName(TEXT("AdjustReactor"));
	}

	AShipInteractableBase* Best = nullptr;
	float BestScore = -1.f;
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AShipInteractableBase> It(World); It; ++It)
		{
			if (It->ActionId != DesiredAction)
			{
				continue;
			}
			const float D = FVector::Dist(GetOwner()->GetActorLocation(), It->GetActorLocation());
			const float Score = 1000.f - D;
			if (Score > BestScore)
			{
				BestScore = Score;
				Best = *It;
			}
		}
	}
	return Best;
}

void UCrewBotBrainComponent::TryMoveAndInteract(AShipInteractableBase* Target)
{
	APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn || !Target)
	{
		return;
	}
	AAIController* AI = Cast<AAIController>(Pawn->GetController());
	if (!AI)
	{
		return;
	}
	const float Dist = FVector::Dist(Pawn->GetActorLocation(), Target->GetActorLocation());
	if (Dist <= AcceptableInteractDistance)
	{
		Target->ExecuteInteract(AI);
		return;
	}
	AI->MoveToActor(Target, AcceptableInteractDistance - 10.f);
}

void UCrewBotBrainComponent::Think()
{
	AShipInteractableBase* Target = FindBestInteractable();
	if (Target)
	{
		TryMoveAndInteract(Target);
	}
}
