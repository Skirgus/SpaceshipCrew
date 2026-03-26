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
 * Single-player MVP: spawns one human at PlayerRoleSlotIndex and bots for other mandatory roles.
 * Ship systems are unchanged for future coop (see IShipCrewSlotPossession).
 */
UCLASS()
class SPACESHIPCREW_API AShipGameMode : public AGameModeBase, public IShipCrewSlotPossession
{
	GENERATED_BODY()

public:
	AShipGameMode();

	/** Optional manifest asset; if null, MandatoryRoles must be set in editor or defaults are created at runtime. */
	UPROPERTY(EditAnywhere, Category = "Crew")
	TObjectPtr<UShipCrewManifest> CrewManifest;

	/** If CrewManifest is null, these roles are used (may be empty → runtime defaults). */
	UPROPERTY(EditAnywhere, Category = "Crew")
	TArray<TObjectPtr<UCrewRoleDefinition>> MandatoryRoles;

	/** Which slot the local human occupies (0-based). */
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
