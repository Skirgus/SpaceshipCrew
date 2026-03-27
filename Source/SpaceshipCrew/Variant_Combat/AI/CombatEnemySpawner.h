// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CombatActivatable.h"
#include "CombatEnemySpawner.generated.h"

class UCapsuleComponent;
class UArrowComponent;
class ACombatEnemy;

/**
 * A basic актор в charge из спавнing Враг Персонажs и monitoring their deaths.
 * Enemies will be спавнed one через one, и спавнer will wait until враг dies до спавнing a new one.
 * спавнer can be remotely activated through ICombatActivatable интерфейс
 * когда последний спавнed враг dies, спавнer can also activate other ICombatActivatables
 */
UCLASS(abstract)
class ACombatEnemySpawner : public AActor, public ICombatActivatable
{
	GENERATED_BODY()
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCapsuleComponent* SpawnCapsule;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UArrowComponent* SpawnDirection;

protected:

	/** Тип врага для спавна */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy Spawner")
	TSubclassOf<ACombatEnemy> EnemyClass;

	/** если true, первый враг will be спавнed as soon as game starts */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy Spawner")
	bool bShouldSpawnEnemiesImmediately = true;

	/** Time для wait до спавнing первый враг на game начать */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy Spawner", meta = (ClampMin = 0, ClampMax = 10))
	float InitialSpawnDelay = 5.0f;

	/** Количество врагов для спавна */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy Spawner", meta = (ClampMin = 0, ClampMax = 100))
	int32 SpawnCount = 1;

	/** Time для wait до спавнing следующий враг после текущий one dies */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy Spawner", meta = (ClampMin = 0, ClampMax = 10))
	float RespawnDelay = 5.0f;

	/** Time для wait после этот спавнer is depleted до activating актор список */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Activation", meta = (ClampMin = 0, ClampMax = 10))
	float ActivationDelay = 1.0f;

	/** список из акторы для activate после последний враг dies */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Activation")
	TArray<AActor*> ActorsToActivateWhenDepleted;

	/** Flag для убедиться этот is только activated once */
	bool bHasBeenActivated = false;

	/** таймер для спавн враги после a delay */
	FTimerHandle SpawnTimer;

public:	
	
	/** Конструктор */
	ACombatEnemySpawner();

public:

	/** Инициализация */
	virtual void BeginPlay() override;

	/** Очистка */
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

protected:

	/** Создаёт врага и подписывается на его событие смерти */
	void SpawnEnemy();

	/** Вызывается, когда заспавненный враг погиб */
	UFUNCTION()
	void OnEnemyDied();

	/** Вызывается после гибели последнего заспавненного врага */
	void SpawnerDepleted();

public:

	// ~begin ICombatActivatable интерфейс

	/** Переключает состояние спавнера */
	UFUNCTION(BlueprintCallable, Category="Activatable")
	virtual void ToggleInteraction(AActor* ActivationInstigator) override;

	/** Активирует спавнер */
	UFUNCTION(BlueprintCallable, Category="Activatable")
	virtual void ActivateInteraction(AActor* ActivationInstigator) override;

	/** DeАктивирует спавнер */
	UFUNCTION(BlueprintCallable, Category="Activatable")
	virtual void DeactivateInteraction(AActor* ActivationInstigator) override;

	// ~end IActivatable интерфейс
};




