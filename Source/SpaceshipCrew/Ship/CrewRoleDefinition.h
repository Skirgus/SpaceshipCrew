// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CrewRoleDefinition.generated.h"

/**
 * Определяет роль экипажа: отображаемые данные и станции корабля, доступные этой роли.
 * Интерактивные станции сравнивают RequiredPermission с AllowedStationPermissions.
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

	/** Например: Reactor, Helm, Medical — должно совпадать с ShipInteractableBase::RequiredPermission. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Role")
	TSet<FName> AllowedStationPermissions;

	UFUNCTION(BlueprintPure, Category = "Role")
	bool CanUseStation(FName StationPermission) const
	{
		return StationPermission.IsNone() || AllowedStationPermissions.Contains(StationPermission);
	}
};

