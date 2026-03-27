// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ShipCrewSlotPossession.h"
#include "ShipGameMode.generated.h"

class APlayerController;
class AShipActor;
class AShipCrewCharacter;
class UCrewRoleDefinition;
class UShipCrewManifest;

/**
 * MVP для одиночной игры: спавнит одного игрока в слоте PlayerRoleSlotIndex и ботов на остальных обязательных ролях.
 * Системы корабля остаются неизменными для будущего коопа (см. IShipCrewSlotPossession).
 */
UCLASS()
class SPACESHIPCREW_API AShipGameMode : public AGameModeBase, public IShipCrewSlotPossession
{
	GENERATED_BODY()

public:
	AShipGameMode();

	/** Необязательный ассет манифеста; если не задан, MandatoryRoles задаются в редакторе или создаются по умолчанию во время выполнения. */
	UPROPERTY(EditAnywhere, Category = "Crew")
	TObjectPtr<UShipCrewManifest> CrewManifest;

	/** Если CrewManifest не задан, используются эти роли (может быть пусто -> runtime-значения по умолчанию). */
	UPROPERTY(EditAnywhere, Category = "Crew")
	TArray<TObjectPtr<UCrewRoleDefinition>> MandatoryRoles;

	/** Какой слот занимает локальный игрок (индексация с нуля). */
	UPROPERTY(EditAnywhere, Category = "Crew")
	int32 PlayerRoleSlotIndex = 0;

	UPROPERTY(EditAnywhere, Category = "Crew")
	TSubclassOf<AShipCrewCharacter> CrewPawnClass;

	virtual void BeginPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual APawn* SpawnDefaultPawnFor(AController* NewPlayer, AActor* StartSpot) override;

	// IShipCrewSlotPossession
	virtual void PossessCrewSlot(APlayerController* Player, int32 SlotIndex) override;

protected:
	void EnsureDefaultRoles();
	void EnsureCrewSpawned();
	void RebuildGameStateCrewSlots();

	AShipActor* FindShipActor() const;
	FTransform GetSpawnTransformForSlot(int32 SlotIndex, AShipActor* Ship, const FVector& FallbackLocation) const;

	UPROPERTY()
	TArray<TObjectPtr<AShipCrewCharacter>> CrewPawns;

	bool bCrewSpawned = false;
};

