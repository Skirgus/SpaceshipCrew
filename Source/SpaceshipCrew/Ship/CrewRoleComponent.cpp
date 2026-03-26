// Copyright Epic Games, Inc. All Rights Reserved.

#include "CrewRoleComponent.h"
#include "CrewRoleDefinition.h"
#include "Net/UnrealNetwork.h"

UCrewRoleComponent::UCrewRoleComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = false;
}

void UCrewRoleComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UCrewRoleComponent, RoleDefinition);
}

void UCrewRoleComponent::SetRoleDefinition(UCrewRoleDefinition* InRole)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		RoleDefinition = InRole;
	}
}

bool UCrewRoleComponent::CanUseStation(FName StationPermission) const
{
	if (!RoleDefinition)
	{
		return false;
	}
	return RoleDefinition->CanUseStation(StationPermission);
}

void UCrewRoleComponent::OnRep_Role()
{
}
