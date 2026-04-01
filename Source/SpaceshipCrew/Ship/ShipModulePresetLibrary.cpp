// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShipModulePresetLibrary.h"

namespace
{
struct FModulePresetSpec
{
	FName DefinitionId;
	FText DisplayName;
	EShipModuleType ModuleType = EShipModuleType::Custom;
	int32 CapabilitiesMask = 0;
};

FModulePresetSpec GetPresetSpec(EShipModulePresetKind Kind)
{
	switch (Kind)
	{
	case EShipModulePresetKind::Reactor:
		return {
			FName(TEXT("ReactorModule")),
			FText::FromString(TEXT("Reactor Module")),
			EShipModuleType::Reactor,
			static_cast<int32>(EShipModuleCapability::ReactorCore) | static_cast<int32>(EShipModuleCapability::FuelStorage)
		};
	case EShipModulePresetKind::Engine:
		return {
			FName(TEXT("EngineModule")),
			FText::FromString(TEXT("Engine Module")),
			EShipModuleType::Engine,
			static_cast<int32>(EShipModuleCapability::EngineCore)
		};
	case EShipModulePresetKind::Bridge:
		return {
			FName(TEXT("BridgeModule")),
			FText::FromString(TEXT("Bridge Module")),
			EShipModuleType::Bridge,
			static_cast<int32>(EShipModuleCapability::BridgeCore)
		};
	case EShipModulePresetKind::Airlock:
		return {
			FName(TEXT("AirlockModule")),
			FText::FromString(TEXT("Airlock Module")),
			EShipModuleType::Airlock,
			static_cast<int32>(EShipModuleCapability::AirlockCore)
		};
	case EShipModulePresetKind::OxygenTank:
		return {
			FName(TEXT("OxygenTankModule")),
			FText::FromString(TEXT("Oxygen Tank Module")),
			EShipModuleType::Custom,
			static_cast<int32>(EShipModuleCapability::OxygenStorage)
		};
	case EShipModulePresetKind::FuelTank:
		return {
			FName(TEXT("FuelTankModule")),
			FText::FromString(TEXT("Fuel Tank Module")),
			EShipModuleType::Custom,
			static_cast<int32>(EShipModuleCapability::FuelStorage)
		};
	default:
		return {};
	}
}
} // namespace

bool UShipModulePresetLibrary::ApplyPresetToDefinition(UShipModuleDefinition* Definition, EShipModulePresetKind PresetKind)
{
	if (!Definition)
	{
		return false;
	}

	const FModulePresetSpec Spec = GetPresetSpec(PresetKind);
	if (Spec.DefinitionId.IsNone())
	{
		return false;
	}

	Definition->ModuleDefinitionId = Spec.DefinitionId;
	Definition->DisplayName = Spec.DisplayName;
	Definition->ModuleType = Spec.ModuleType;
	Definition->CapabilitiesMask = Spec.CapabilitiesMask;
	Definition->GridSize = FIntVector(1, 1, 1);
	Definition->ConnectionPoints.Reset();
	Definition->bHasInteriorVolume = true;
	Definition->bAllowConnectionsOnAllSides = false;

	if (PresetKind == EShipModulePresetKind::Engine)
	{
		Definition->bHasInteriorVolume = false;
		FShipModuleConnectionPoint P;
		P.Face = EShipModuleFace::NegX;
		P.Slot = 0.5f;
		Definition->ConnectionPoints.Add(P);
	}
	else if (PresetKind == EShipModulePresetKind::FuelTank)
	{
		Definition->bHasInteriorVolume = false;
		Definition->bAllowConnectionsOnAllSides = true;
	}

	Definition->MarkPackageDirty();
	return true;
}

bool UShipModulePresetLibrary::BuildStarterShipConfig(
	UShipConfigAsset* Config,
	UShipModuleDefinition* ReactorDefinition,
	UShipModuleDefinition* EngineDefinition,
	UShipModuleDefinition* BridgeDefinition,
	UShipModuleDefinition* AirlockDefinition,
	UShipModuleDefinition* OxygenTankDefinition,
	UShipModuleDefinition* FuelTankDefinition)
{
	if (!Config || !ReactorDefinition || !EngineDefinition || !BridgeDefinition || !AirlockDefinition || !OxygenTankDefinition || !FuelTankDefinition)
	{
		return false;
	}

	Config->Modules.Reset();
	Config->Connections.Reset();
	Config->ConfigId = FName(TEXT("StarterShip"));
	Config->OxygenReserve = 100.f;
	Config->FuelReserve = 100.f;

	auto AddModule = [Config](FName ModuleId, UShipModuleDefinition* Def, const FIntVector& GridPosition)
	{
		FShipModuleConfig M;
		M.ModuleId = ModuleId;
		M.ModuleDefinition = Def;
		M.ModuleType = Def->ModuleType;
		M.GridPosition = GridPosition;
		M.OxygenLevel = 21.f;
		M.FireHotspotCount = 0;
		M.BreachSeverity = 0.f;
		Config->Modules.Add(M);
	};

	AddModule(FName(TEXT("Reactor")), ReactorDefinition, FIntVector(0, 0, 0));
	AddModule(FName(TEXT("Engine")), EngineDefinition, FIntVector(1, 0, 0));
	AddModule(FName(TEXT("Bridge")), BridgeDefinition, FIntVector(2, 0, 0));
	AddModule(FName(TEXT("Airlock")), AirlockDefinition, FIntVector(3, 0, 0));
	AddModule(FName(TEXT("OxygenTank")), OxygenTankDefinition, FIntVector(1, 1, 0));
	AddModule(FName(TEXT("FuelTank")), FuelTankDefinition, FIntVector(0, 1, 0));

	auto AddConnection = [Config](FName A, FName B)
	{
		FShipConnectionConfig C;
		C.ModuleA = A;
		C.ModuleB = B;
		C.bOpenByDefault = true;
		Config->Connections.Add(C);
	};

	AddConnection(FName(TEXT("Reactor")), FName(TEXT("Engine")));
	AddConnection(FName(TEXT("Engine")), FName(TEXT("Bridge")));
	AddConnection(FName(TEXT("Bridge")), FName(TEXT("Airlock")));
	AddConnection(FName(TEXT("Reactor")), FName(TEXT("FuelTank")));
	AddConnection(FName(TEXT("Engine")), FName(TEXT("OxygenTank")));

	Config->MarkPackageDirty();
	return true;
}

