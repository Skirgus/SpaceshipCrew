#include "DamagedHullPanel.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Pawn.h"

ADamagedHullPanel::ADamagedHullPanel()
{
	PrimaryActorTick.bCanEverTick = true;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(MeshComponent);
	MeshComponent->SetCollisionProfileName(TEXT("BlockAllDynamic"));
}

void ADamagedHullPanel::SetDisplayMesh(UStaticMesh* InMesh)
{
	if (MeshComponent && InMesh)
	{
		MeshComponent->SetStaticMesh(InMesh);
	}
}

void ADamagedHullPanel::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bRepairHolding || RepairState != EHullPanelRepairState::Damaged)
	{
		return;
	}

	const float Duration = FMath::Max(0.1f, RepairDurationSeconds);
	RepairProgress = FMath::Clamp(RepairProgress + DeltaSeconds / Duration, 0.0f, 1.0f);
	if (RepairProgress >= 1.0f - KINDA_SMALL_NUMBER)
	{
		SetRepairState(EHullPanelRepairState::Repaired);
		bRepairHolding = false;
		RepairingPawn = nullptr;
		OnPanelRepaired.Broadcast();
	}
}

bool ADamagedHullPanel::CanInteract_Implementation(APawn* InstigatorPawn) const
{
	return InstigatorPawn && RepairState == EHullPanelRepairState::Damaged;
}

FText ADamagedHullPanel::GetInteractPrompt_Implementation(APawn* InstigatorPawn) const
{
	if (RepairState == EHullPanelRepairState::Damaged)
	{
		return NSLOCTEXT("SpaceshipCrew", "Panel_Repair", "Ремонтировать (удерживать)");
	}
	return FText::GetEmpty();
}

void ADamagedHullPanel::Interact_Implementation(APawn* InstigatorPawn)
{
	BeginRepair(InstigatorPawn);
}

void ADamagedHullPanel::BeginRepair(APawn* InstigatorPawn)
{
	if (!CanInteract_Implementation(InstigatorPawn))
	{
		return;
	}
	bRepairHolding = true;
	RepairingPawn = InstigatorPawn;
}

void ADamagedHullPanel::StopRepair()
{
	bRepairHolding = false;
	RepairingPawn = nullptr;
}

void ADamagedHullPanel::SetRepairState(EHullPanelRepairState NewState)
{
	RepairState = NewState;
	if (NewState == EHullPanelRepairState::Damaged)
	{
		RepairProgress = 0.0f;
	}
	else if (NewState == EHullPanelRepairState::Repaired || NewState == EHullPanelRepairState::Intact)
	{
		RepairProgress = 1.0f;
		if (NewState == EHullPanelRepairState::Repaired)
		{
			if (UStaticMesh* Intact = LoadObject<UStaticMesh>(
					nullptr,
					TEXT("/Game/Meshes/EngineerTraining/SM_HullPanel_Intact.SM_HullPanel_Intact")))
			{
				SetDisplayMesh(Intact);
			}
		}
	}
}
