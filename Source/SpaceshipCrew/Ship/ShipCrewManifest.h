// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ShipCrewManifest.generated.h"

class UCrewRoleDefinition;

/** Упорядоченный список обязательных ролей для конфигурации корабля (индекс слота = индекс в массиве). */
UCLASS(BlueprintType)
class SPACESHIPCREW_API UShipCrewManifest : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crew")
	TArray<TObjectPtr<UCrewRoleDefinition>> MandatoryRoles;
};

