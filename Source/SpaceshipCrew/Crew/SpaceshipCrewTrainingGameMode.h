#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SpaceshipCrewTrainingGameMode.generated.h"

class AShipBuilderModulePreviewActor;

/**
 * Тренировки: загружает учебный корабль (чертёж TrainingVessel) и пеший SpaceshipCrewCharacter.
 * Сценарий класса (инженер и др.) вешается поверх того же корпуса — см. docs/TRAINING_SHIP_CONCEPT_RU.md.
 */
UCLASS()
class SPACESHIPCREW_API ASpaceshipCrewTrainingGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ASpaceshipCrewTrainingGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
	virtual void RestartPlayer(AController* NewPlayer) override;

	/** ShipId в Content/Data/Ships (по умолчанию TrainingVessel). */
	UPROPERTY(EditDefaultsOnly, Category = "Training")
	FName TrainingShipId = FName(TEXT("TrainingVessel"));

	/** InstanceId модуля, где спавн и станции инженера. */
	UPROPERTY(EditDefaultsOnly, Category = "Training")
	FName EngineeringModuleInstanceId = FName(TEXT("TV_Engineering"));

protected:
	void EnsureTrainingLayout();
	bool SpawnTrainingShipHull();
	void PlaceTrainingPropsAround(const FVector& EngineeringCenter);

	UPROPERTY(Transient)
	TObjectPtr<AActor> TrainingPlayerStart = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<AShipBuilderModulePreviewActor> TrainingHull = nullptr;

	FTransform TrainingSpawnTransform = FTransform::Identity;
	FVector EngineeringModuleCenter = FVector::ZeroVector;
};
