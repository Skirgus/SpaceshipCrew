// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CrewTypes.generated.h"

class UCrewRoleDefinition;
class APawn;

/** Кто управляет слотом экипажа (человек, ИИ или резерв под будущего сетевого игрока). */
UENUM(BlueprintType)
enum class ECrewControllerKind : uint8
{
	Bot UMETA(DisplayName = "Bot"),
	Human UMETA(DisplayName = "Human"),
	ReservedForPlayer UMETA(DisplayName = "ReservedForPlayer")
};

/** Сериализуемые данные слота, реплицируемые в GameState для UI и будущего лобби. */
USTRUCT(BlueprintType)
struct SPACESHIPCREW_API FCrewSlotReplicationData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Crew")
	int32 SlotIndex = INDEX_NONE;

	/** Стабильный идентификатор роли (или NAME_None, если роль не назначена). */
	UPROPERTY(BlueprintReadOnly, Category = "Crew")
	FName RoleId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Crew")
	ECrewControllerKind ControllerKind = ECrewControllerKind::Bot;

	UPROPERTY(BlueprintReadOnly, Category = "Crew")
	TObjectPtr<APawn> AssignedPawn = nullptr;
};

