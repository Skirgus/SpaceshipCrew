// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShipSystemsComponent.h"
#include "CrewRoleComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"

UShipSystemsComponent::UShipSystemsComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void UShipSystemsComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UShipSystemsComponent, ReactorOutput);
	DOREPLIFETIME(UShipSystemsComponent, OxygenLevel);
	DOREPLIFETIME(UShipSystemsComponent, HullIntegrity);
	DOREPLIFETIME(UShipSystemsComponent, FireIntensity);
}

void UShipSystemsComponent::OnRep_Snapshot()
{
}

void UShipSystemsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		SimulateSystems(DeltaTime);
	}
}

void UShipSystemsComponent::SimulateSystems(float DeltaTime)
{
	// Simple passive model: fire consumes oxygen; low oxygen damages hull; reactor mitigates fire slowly when high output
	if (FireIntensity > 0.01f)
	{
		OxygenLevel = FMath::Clamp(OxygenLevel - 0.02f * FireIntensity * DeltaTime, 0.f, 1.f);
		FireIntensity = FMath::Clamp(FireIntensity - 0.05f * ReactorOutput * DeltaTime, 0.f, 1.f);
	}
	else
	{
		OxygenLevel = FMath::Clamp(OxygenLevel + 0.01f * ReactorOutput * DeltaTime, 0.f, 1.f);
	}
	if (OxygenLevel < 0.2f)
	{
		HullIntegrity = FMath::Clamp(HullIntegrity - 0.01f * DeltaTime, 0.f, 1.f);
	}
}

bool UShipSystemsComponent::ResolveCrewRole(AController* Issuer, UCrewRoleComponent*& OutRole)
{
	OutRole = nullptr;
	if (!Issuer)
	{
		return false;
	}
	APawn* Pawn = Issuer->GetPawn();
	if (!Pawn)
	{
		return false;
	}
	OutRole = Pawn->FindComponentByClass<UCrewRoleComponent>();
	return OutRole != nullptr;
}

bool UShipSystemsComponent::CanIssuerUseStation(AController* Issuer, FName StationPermission) const
{
	UCrewRoleComponent* Role = nullptr;
	if (!ResolveCrewRole(const_cast<AController*>(Issuer), Role) || !Role)
	{
		return false;
	}
	return Role->CanUseStation(StationPermission);
}

bool UShipSystemsComponent::ApplyAuthorizedAction(AController* Issuer, FName StationPermission, FName ActionId, float Magnitude)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return false;
	}
	return InternalApplyAction(Issuer, StationPermission, ActionId, Magnitude);
}

bool UShipSystemsComponent::InternalApplyAction(AController* Issuer, FName StationPermission, FName ActionId, float Magnitude)
{
	UCrewRoleComponent* Role = nullptr;
	if (!ResolveCrewRole(Issuer, Role) || !Role || !Role->CanUseStation(StationPermission))
	{
		return false;
	}

	// MVP discrete actions — extend with DataTable later
	if (ActionId == FName(TEXT("AdjustReactor")))
	{
		ReactorOutput = FMath::Clamp(ReactorOutput + 0.1f * Magnitude, 0.f, 1.f);
		return true;
	}
	if (ActionId == FName(TEXT("VentAtmosphere")))
	{
		OxygenLevel = FMath::Clamp(OxygenLevel - 0.05f * Magnitude, 0.f, 1.f);
		return true;
	}
	if (ActionId == FName(TEXT("RepairHull")))
	{
		HullIntegrity = FMath::Clamp(HullIntegrity + 0.1f * Magnitude, 0.f, 1.f);
		return true;
	}
	if (ActionId == FName(TEXT("FightFire")))
	{
		FireIntensity = FMath::Clamp(FireIntensity - 0.2f * Magnitude, 0.f, 1.f);
		return true;
	}
	if (ActionId == FName(TEXT("StartFireEvent"))) // debug / mission director
	{
		FireIntensity = FMath::Clamp(FireIntensity + 0.25f * Magnitude, 0.f, 1.f);
		return true;
	}

	return false;
}
