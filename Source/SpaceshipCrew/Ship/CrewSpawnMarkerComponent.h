// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "CrewSpawnMarkerComponent.generated.h"

/**
 * Точка спавна члена экипажа на корабле: добавь на BP_Ship, выставь в вьюпорте, задай Role Id как у роли (Captain, Engineer, ...).
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class SPACESHIPCREW_API UCrewSpawnMarkerComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UCrewSpawnMarkerComponent();

	/** Совпадает с Role Id на UCrewRoleDefinition. Пусто = «любая роль»: заполняется после точных совпадений, пока есть свободные маркеры. */
	UPROPERTY(EditAnywhere, Category = "Crew")
	FName RoleId = NAME_None;
};
