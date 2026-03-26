// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CrewRoleDefinition.generated.h"

/**
 * Defines a crew role: display info and which ship stations this role may operate.
 * Station interactables compare RequiredPermission against AllowedStationPermissions.
 */
UCLASS(BlueprintType)
class SPACESHIPCREW_API UCrewRoleDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Role")
	FName RoleId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Role")
	FText DisplayName;

	/** e.g. Reactor, Helm, Medical — must match ShipInteractableBase::RequiredPermission. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Role")
	TSet<FName> AllowedStationPermissions;

	UFUNCTION(BlueprintPure, Category = "Role")
	bool CanUseStation(FName StationPermission) const
	{
		return StationPermission.IsNone() || AllowedStationPermissions.Contains(StationPermission);
	}
};
