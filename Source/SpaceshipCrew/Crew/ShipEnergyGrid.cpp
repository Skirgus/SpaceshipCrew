#include "ShipEnergyGrid.h"

float UShipEnergyGrid::NormalizeTriplet(float& A, float& B, float& C)
{
	A = FMath::Max(0.0f, A);
	B = FMath::Max(0.0f, B);
	C = FMath::Max(0.0f, C);
	const float Sum = A + B + C;
	if (Sum <= KINDA_SMALL_NUMBER)
	{
		A = B = C = 100.0f / 3.0f;
		return 100.0f;
	}
	A = A / Sum * 100.0f;
	B = B / Sum * 100.0f;
	C = 100.0f - A - B;
	return 100.0f;
}

void UShipEnergyGrid::SetShares(float InShields, float InWeapons, float InEngines)
{
	float S = InShields;
	float W = InWeapons;
	float E = InEngines;
	NormalizeTriplet(S, W, E);
	Shares.Shields = S;
	Shares.Weapons = W;
	Shares.Engines = E;
}

void UShipEnergyGrid::ApplyDefensePreset()
{
	// Пресет «Оборона»: щиты не ниже учебного порога 50%.
	SetShares(55.0f, 25.0f, 20.0f);
}

bool UShipEnergyGrid::MeetsShieldThreshold(float MinShieldsPercent) const
{
	return Shares.Shields + KINDA_SMALL_NUMBER >= MinShieldsPercent;
}
