// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CrewTypes.generated.h"

class UCrewRoleDefinition;
class APawn;

/** Who owns a crew slot (human, AI, or reserved for future net player). */
UENUM(BlueprintType)
enum class ECrewControllerKind : uint8
{
	Bot UMETA(DisplayName = "Bot"),
	Human UMETA(DisplayName = "Human"),
	ReservedForPlayer UMETA(DisplayName = "ReservedForPlayer")
};

/** Serializable slot data replicated on GameState for UI and future lobby. */
USTRUCT(BlueprintType)
struct SPACESHIPCREW_API FCrewSlotReplicationData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Crew")
	int32 SlotIndex = INDEX_NONE;

	/** Stable id from role definition (or NAME_None if unassigned). */
	UPROPERTY(BlueprintReadOnly, Category = "Crew")
	FName RoleId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Crew")
	ECrewControllerKind ControllerKind = ECrewControllerKind::Bot;

	UPROPERTY(BlueprintReadOnly, Category = "Crew")
	TObjectPtr<APawn> AssignedPawn = nullptr;
};
