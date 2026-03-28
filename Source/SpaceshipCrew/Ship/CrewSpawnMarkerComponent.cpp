// Copyright Epic Games, Inc. All Rights Reserved.

#include "CrewSpawnMarkerComponent.h"

UCrewSpawnMarkerComponent::UCrewSpawnMarkerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetMobility(EComponentMobility::Static);
#if WITH_EDITORONLY_DATA
	bVisualizeComponent = true;
#endif
}
