#include "EngineerEnergyConsole.h"

#include "ShipEnergyGrid.h"

AEngineerEnergyConsole::AEngineerEnergyConsole()
{
	StationRole = ECrewStationRole::Engineer;
	EnergyGrid = CreateDefaultSubobject<UShipEnergyGrid>(TEXT("EnergyGrid"));
}

void AEngineerEnergyConsole::BeginPlay()
{
	Super::BeginPlay();
	// Учебный разбаланс: мало щитов — сценарий потребует пресет обороны.
	if (EnergyGrid)
	{
		EnergyGrid->SetShares(20.0f, 40.0f, 40.0f);
	}
}

void AEngineerEnergyConsole::SetEnergyShares(float Shields, float Weapons, float Engines)
{
	if (!EnergyGrid)
	{
		return;
	}
	EnergyGrid->SetShares(Shields, Weapons, Engines);
	OnEnergySharesChanged.Broadcast();
}

void AEngineerEnergyConsole::ApplyDefensePreset()
{
	if (!EnergyGrid)
	{
		return;
	}
	EnergyGrid->ApplyDefensePreset();
	OnEnergySharesChanged.Broadcast();
}
