// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CrewLobbySubsystem.generated.h"

/** Заглушка под будущее лобби/сессии; оставляет точку расширения без добавления сетевого кода в MVP. */
UCLASS()
class SPACESHIPCREW_API UCrewLobbySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Crew|Lobby")
	void Stub_RegisterFutureLobbyHooks() {}
};

