// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CrewLobbySubsystem.generated.h"

/** Placeholder for future lobby / session flow; keeps extension point without shipping net code in MVP. */
UCLASS()
class SPACESHIPCREW_API UCrewLobbySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Crew|Lobby")
	void Stub_RegisterFutureLobbyHooks() {}
};
