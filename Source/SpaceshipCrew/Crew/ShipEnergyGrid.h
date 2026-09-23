#pragma once

#include "CoreMinimal.h"
#include "ShipEnergyGrid.generated.h"

/**
 * Доли энергосети корабля (щиты / оружие / двигатели). Сумма всегда 100%.
 * Чистая модель для UI станции инженера и автотестов.
 */
USTRUCT(BlueprintType)
struct SPACESHIPCREW_API FShipEnergyShares
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Energy", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float Shields = 34.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Energy", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float Weapons = 33.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Energy", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float Engines = 33.0f;
};

UCLASS(BlueprintType)
class SPACESHIPCREW_API UShipEnergyGrid : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Crew|Energy")
	FShipEnergyShares GetShares() const { return Shares; }

	/** Задать доли с нормализацией до 100%. */
	UFUNCTION(BlueprintCallable, Category = "Crew|Energy")
	void SetShares(float InShields, float InWeapons, float InEngines);

	UFUNCTION(BlueprintCallable, Category = "Crew|Energy")
	void ApplyDefensePreset();

	UFUNCTION(BlueprintPure, Category = "Crew|Energy")
	bool MeetsShieldThreshold(float MinShieldsPercent) const;

	UFUNCTION(BlueprintPure, Category = "Crew|Energy")
	static float NormalizeTriplet(float& A, float& B, float& C);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crew|Energy")
	FShipEnergyShares Shares;
};
