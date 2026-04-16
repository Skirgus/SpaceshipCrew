#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SpectatorPawn.h"
#include "SpaceshipShipBuilderSpectatorPawn.generated.h"

/**
 * Свободная камера конструктора: не опускается ниже заданной высоты (уровень «пола» в мире).
 */
UCLASS()
class SPACESHIPCREW_API ASpaceshipShipBuilderSpectatorPawn : public ASpectatorPawn
{
	GENERATED_BODY()

public:
	ASpaceshipShipBuilderSpectatorPawn();

	virtual void PostInitializeComponents() override;
	virtual void Tick(float DeltaSeconds) override;

protected:
	/** Минимальная координата Z актёра (см), ниже — не опускаемся (пол обычно 0). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ShipBuilder|Camera")
	float MinWorldZ = 20.0f;

private:
	void ClampToFloorHeight();
};
