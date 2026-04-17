#pragma once

#include "CoreMinimal.h"

#include "ShipBuilder/ShipBuilderDraftTypes.h"

class UShipModuleCatalog;

/** Идентификаторы параметров для UI (расширяемый список). */
namespace ShipBuilderParameterIds
{
	inline const FName MassKg(TEXT("MassKg"));
	inline const FName Hull(TEXT("Hull"));
	inline const FName Mobility(TEXT("Mobility"));
	inline const FName ReactorOutput(TEXT("ReactorOutput"));
	inline const FName PowerDemand(TEXT("PowerDemand"));
}

/** Одна строка снимка параметров корабля. */
struct FShipBuilderParameterEntry
{
	FName Id;
	FText Label;
	double Value = 0.0;
	/** Если true, рост значения при сравнении считается улучшением (цвет в UI). */
	bool bHigherIsBetter = true;
};

struct FShipBuilderParameterSnapshot
{
	TArray<FShipBuilderParameterEntry> Entries;
};

/**
 * Заглушечный расчёт агрегатов для экрана конструктора. Формулы заменяемы в одном месте.
 */
struct FShipBuilderParameterEvaluator
{
	static void BuildOrderedParameterIds(TArray<FName>& OutOrderedIds);

	static void ComputeSnapshot(
		const FShipBuilderDraftConfig& Draft,
		const UShipModuleCatalog& Catalog,
		FShipBuilderParameterSnapshot& OutSnapshot);

	/** Добавляет модуль PreviewModuleId в конец черновика только для расчёта (не мутирует Draft). */
	static void ComputeSnapshotWithPreviewAppend(
		const FShipBuilderDraftConfig& Draft,
		FName PreviewModuleId,
		const UShipModuleCatalog& Catalog,
		FShipBuilderParameterSnapshot& OutSnapshot);
};
