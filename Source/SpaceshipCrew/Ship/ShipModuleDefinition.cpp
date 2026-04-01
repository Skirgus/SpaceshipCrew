// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShipModuleDefinition.h"

bool UShipModuleDefinition::HasCapability(EShipModuleCapability Capability) const
{
	return (CapabilitiesMask & static_cast<int32>(Capability)) != 0;
}

TArray<FShipModuleConnectionPoint> UShipModuleDefinition::GetEffectiveConnectionPoints() const
{
	if (ConnectionPoints.Num() > 0)
	{
		return ConnectionPoints;
	}

	TArray<FShipModuleConnectionPoint> Result;

	auto AddFacePoints = [&Result](EShipModuleFace Face, int32 Count)
	{
		const int32 SafeCount = FMath::Max(1, Count);
		for (int32 i = 0; i < SafeCount; ++i)
		{
			FShipModuleConnectionPoint P;
			P.Face = Face;
			P.Slot = (static_cast<float>(i) + 0.5f) / static_cast<float>(SafeCount);
			Result.Add(P);
		}
	};

	if (bHasInteriorVolume)
	{
		AddFacePoints(EShipModuleFace::PosX, GridSize.Y);
		AddFacePoints(EShipModuleFace::NegX, GridSize.Y);
		AddFacePoints(EShipModuleFace::PosY, GridSize.X);
		AddFacePoints(EShipModuleFace::NegY, GridSize.X);
		return Result;
	}

	if (bAllowConnectionsOnAllSides)
	{
		AddFacePoints(EShipModuleFace::PosX, 1);
		AddFacePoints(EShipModuleFace::NegX, 1);
		AddFacePoints(EShipModuleFace::PosY, 1);
		AddFacePoints(EShipModuleFace::NegY, 1);
	}
	else
	{
		// По умолчанию для "внешних" модулей одна точка с тыльной стороны.
		AddFacePoints(EShipModuleFace::NegX, 1);
	}

	return Result;
}

