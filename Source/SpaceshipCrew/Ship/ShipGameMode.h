// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ShipCrewSlotPossession.h"
#include "ShipGameMode.generated.h"

class AController;
class APlayerController;
class AShipActor;
class AShipCrewCharacter;
class UCrewRoleDefinition;
class UShipCrewManifest;
class UShipConfigAsset;

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

	/** Конфиг корабля, выбираемый на уровне (fallback на AShipActor::DefaultShipConfig). */
	UPROPERTY(EditAnywhere, Category = "Ship|Config")
	TSoftObjectPtr<UShipConfigAsset> SelectedShipConfig;

	/** Класс корабля для runtime-спавна, если на уровне нет уже размещенного корабля. */
	UPROPERTY(EditAnywhere, Category = "Ship|Config")
	TSubclassOf<AShipActor> ShipActorClass;

	virtual void BeginPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void RestartPlayer(AController* NewPlayer) override;
	virtual APawn* SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot) override;

	// IShipCrewSlotPossession
	virtual void PossessCrewSlot(APlayerController* Player, int32 SlotIndex) override;

protected:
	void EnsureDefaultRoles();
	void EnsureShipSpawnedAndConfigured();
	void EnsureCrewSpawned();
	void RebuildGameStateCrewSlots();

	/** Роль, бот-мозг, AIController для слота после успешного SpawnActor. */
	void FinalizeCrewPawnSpawn(AShipCrewCharacter* Spawned, int32 SlotIndex);

	AShipActor* FindShipActor() const;
	/** Сначала маркеры (точный RoleId, затем пустой RoleId), оставшиеся слоты — смещение по умолчанию от корабля. */
	void BuildCrewSpawnTransforms(AShipActor* Ship, const FVector& FallbackLocation, TArray<FTransform>& OutPerSlot) const;
	void ApplyShipConfigSelection(AShipActor* Ship);

	/** Экипаж уже в EnsureCrewSpawned; если слот игрока пуст — аварийный спавн у StartSpot / PlayerStart. */
	AShipCrewCharacter* SpawnOrRetrieveHumanPawn(AController* NewPlayer, AActor* StartSpot);

	UPROPERTY()
	TArray<TObjectPtr<AShipCrewCharacter>> CrewPawns;

	UPROPERTY()
	TObjectPtr<AShipActor> RuntimeSpawnedShip = nullptr;

	bool bCrewSpawned = false;
};

