#include "ShipBuilderParameterEvaluator.h"

#include "ShipModuleCatalog.h"
#include "ShipModuleDefinition.h"
#include "ShipModuleTypes.h"

namespace ShipBuilderParameterEvaluatorPrivate
{
	static void AccumulateFromDraft(
		const FShipBuilderDraftConfig& Draft,
		const UShipModuleCatalog& Catalog,
		double& OutMass,
		int32& OutEngineCount,
		bool& bOutHasReactor)
	{
		OutMass = 0.0;
		OutEngineCount = 0;
		bOutHasReactor = false;
		for (const FName ModuleId : Draft.ModuleIds)
		{
			if (const UShipModuleDefinition* Def = Catalog.FindModuleById(ModuleId))
			{
				OutMass += static_cast<double>(Def->Mass);
				if (Def->ModuleType == EShipModuleType::Engine)
				{
					++OutEngineCount;
				}
				if (Def->ModuleType == EShipModuleType::Reactor)
				{
					bOutHasReactor = true;
				}
			}
		}
	}

	static void FillSnapshotFromAggregates(
		double Mass,
		int32 EngineCount,
		bool bHasReactor,
		int32 ModuleCount,
		FShipBuilderParameterSnapshot& Out)
	{
		Out.Entries.Reset();
		const double HullValue = 100.0 + 2.0 * static_cast<double>(ModuleCount);
		const double MobilityValue = FMath::Clamp(
			50.0 - Mass / 25.0 + static_cast<double>(EngineCount) * 6.0,
			0.0,
			100.0);
		const double ReactorOut = bHasReactor ? 100.0 : 0.0;
		const double PowerReq = Mass * 0.12;

		auto Add = [&](const FName& Id, const FText& Label, double Value, bool bHigherBetter)
		{
			FShipBuilderParameterEntry Entry;
			Entry.Id = Id;
			Entry.Label = Label;
			Entry.Value = Value;
			Entry.bHigherIsBetter = bHigherBetter;
			Out.Entries.Add(MoveTemp(Entry));
		};

		Add(ShipBuilderParameterIds::MassKg, NSLOCTEXT("SpaceshipCrew", "Param_MassKg", "МАССА (КГ)"), Mass, false);
		Add(ShipBuilderParameterIds::Hull, NSLOCTEXT("SpaceshipCrew", "Param_Hull", "КОРПУС"), HullValue, true);
		Add(ShipBuilderParameterIds::Mobility, NSLOCTEXT("SpaceshipCrew", "Param_Mobility", "МОБИЛЬНОСТЬ"), MobilityValue, true);
		Add(ShipBuilderParameterIds::ReactorOutput, NSLOCTEXT("SpaceshipCrew", "Param_ReactorOut", "РЕАКТОР (УСЛ.)"), ReactorOut, true);
		Add(ShipBuilderParameterIds::PowerDemand, NSLOCTEXT("SpaceshipCrew", "Param_PowerDemand", "ПИТАНИЕ (УСЛ.)"), PowerReq, false);
	}
}

void FShipBuilderParameterEvaluator::BuildOrderedParameterIds(TArray<FName>& OutOrderedIds)
{
	OutOrderedIds = {
		ShipBuilderParameterIds::MassKg,
		ShipBuilderParameterIds::Hull,
		ShipBuilderParameterIds::Mobility,
		ShipBuilderParameterIds::ReactorOutput,
		ShipBuilderParameterIds::PowerDemand,
	};
}

void FShipBuilderParameterEvaluator::ComputeSnapshot(
	const FShipBuilderDraftConfig& Draft,
	const UShipModuleCatalog& Catalog,
	FShipBuilderParameterSnapshot& OutSnapshot)
{
	double Mass = 0.0;
	int32 EngineCount = 0;
	bool bReactor = false;
	ShipBuilderParameterEvaluatorPrivate::AccumulateFromDraft(Draft, Catalog, Mass, EngineCount, bReactor);
	ShipBuilderParameterEvaluatorPrivate::FillSnapshotFromAggregates(
		Mass,
		EngineCount,
		bReactor,
		Draft.ModuleIds.Num(),
		OutSnapshot);
}

void FShipBuilderParameterEvaluator::ComputeSnapshotWithPreviewAppend(
	const FShipBuilderDraftConfig& Draft,
	const FName PreviewModuleId,
	const UShipModuleCatalog& Catalog,
	FShipBuilderParameterSnapshot& OutSnapshot)
{
	FShipBuilderDraftConfig Temp = Draft;
	if (!PreviewModuleId.IsNone())
	{
		Temp.ModuleIds.Add(PreviewModuleId);
	}
	ComputeSnapshot(Temp, Catalog, OutSnapshot);
}
