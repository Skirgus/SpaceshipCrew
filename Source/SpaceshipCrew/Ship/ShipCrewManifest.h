// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ShipCrewManifest.generated.h"

class UCrewRoleDefinition;

/** Ordered list of mandatory roles for a ship layout (slot index = array index). */
UCLASS(BlueprintType)
class SPACESHIPCREW_API UShipCrewManifest : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crew")
	TArray<TObjectPtr<UCrewRoleDefinition>> MandatoryRoles;
};
