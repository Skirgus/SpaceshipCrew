#pragma once

#include "CoreMinimal.h"

/** Единичная панель BigModularSciFi / конструктора (см). */
namespace ShipBuilderGrid
{
	inline constexpr float PanelUnitXY = 400.0f;
	inline constexpr float PanelUnitZ = 300.0f;

	inline FIntVector WorldSizeToCellSize(const FVector& WorldSize)
	{
		return FIntVector(
			FMath::Max(1, FMath::RoundToInt(WorldSize.X / PanelUnitXY)),
			FMath::Max(1, FMath::RoundToInt(WorldSize.Y / PanelUnitXY)),
			FMath::Max(1, FMath::RoundToInt(WorldSize.Z / PanelUnitZ)));
	}

	inline FVector CellSizeToWorldSize(const FIntVector& CellSize)
	{
		return FVector(
			static_cast<float>(CellSize.X) * PanelUnitXY,
			static_cast<float>(CellSize.Y) * PanelUnitXY,
			static_cast<float>(CellSize.Z) * PanelUnitZ);
	}

	inline FIntVector GetEffectiveCellSize(const FIntVector& CellSize)
	{
		return FIntVector(
			FMath::Max(1, CellSize.X),
			FMath::Max(1, CellSize.Y),
			FMath::Max(1, CellSize.Z));
	}

	inline bool DoFootprintsOverlap(
		const FIntVector& CornerA,
		const FIntVector& CellsA,
		const FIntVector& CornerB,
		const FIntVector& CellsB)
	{
		return CornerA.X < CornerB.X + CellsB.X
			&& CornerB.X < CornerA.X + CellsA.X
			&& CornerA.Y < CornerB.Y + CellsB.Y
			&& CornerB.Y < CornerA.Y + CellsA.Y
			&& CornerA.Z < CornerB.Z + CellsB.Z
			&& CornerB.Z < CornerA.Z + CellsA.Z;
	}

	inline FIntVector WorldCenterToGridCorner(
		const FVector& Center,
		const FIntVector& CellSize,
		const float GridStepXY = PanelUnitXY,
		const float GridStepZ = PanelUnitZ)
	{
		const FVector CornerWorld = Center - CellSizeToWorldSize(CellSize) * 0.5f;
		return FIntVector(
			FMath::RoundToInt(CornerWorld.X / GridStepXY),
			FMath::RoundToInt(CornerWorld.Y / GridStepXY),
			FMath::RoundToInt(CornerWorld.Z / GridStepZ));
	}

	/** v1→v2: GridPos хранил центр модуля в индексах ячеек. */
	inline FIntVector CenterGridCellToCornerGridCell(const FIntVector& CenterCell, const FIntVector& CellSize)
	{
		return FIntVector(
			CenterCell.X - CellSize.X / 2,
			CenterCell.Y - CellSize.Y / 2,
			CenterCell.Z - CellSize.Z / 2);
	}
}
