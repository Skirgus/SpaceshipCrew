// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShipConfigAsset.h"
#include "ShipModuleDefinition.h"
#include "Containers/Queue.h"

bool UShipConfigAsset::ValidateConfig(TArray<FText>& OutErrors) const
{
	OutErrors.Reset();

	if (Modules.Num() == 0)
	{
		OutErrors.Add(FText::FromString(TEXT("ShipConfig has no modules.")));
		return false;
	}

	if (OxygenReserve <= 0.f)
	{
		OutErrors.Add(FText::FromString(TEXT("OxygenReserve must be > 0.")));
	}

	if (FuelReserve <= 0.f)
	{
		OutErrors.Add(FText::FromString(TEXT("FuelReserve must be > 0.")));
	}

	const bool bHasReactor = Modules.ContainsByPredicate([](const FShipModuleConfig& Module)
	{
		const UShipModuleDefinition* Definition = Module.ModuleDefinition.LoadSynchronous();
		return Definition
			? Definition->HasCapability(EShipModuleCapability::ReactorCore)
			: (Module.ModuleType == EShipModuleType::Reactor);
	});
	const bool bHasEngine = Modules.ContainsByPredicate([](const FShipModuleConfig& Module)
	{
		const UShipModuleDefinition* Definition = Module.ModuleDefinition.LoadSynchronous();
		return Definition
			? Definition->HasCapability(EShipModuleCapability::EngineCore)
			: (Module.ModuleType == EShipModuleType::Engine);
	});
	const bool bHasBridge = Modules.ContainsByPredicate([](const FShipModuleConfig& Module)
	{
		const UShipModuleDefinition* Definition = Module.ModuleDefinition.LoadSynchronous();
		return Definition
			? Definition->HasCapability(EShipModuleCapability::BridgeCore)
			: (Module.ModuleType == EShipModuleType::Bridge);
	});
	const bool bHasAirlock = Modules.ContainsByPredicate([](const FShipModuleConfig& Module)
	{
		const UShipModuleDefinition* Definition = Module.ModuleDefinition.LoadSynchronous();
		return Definition
			? Definition->HasCapability(EShipModuleCapability::AirlockCore)
			: (Module.ModuleType == EShipModuleType::Airlock);
	});
	const bool bHasOxygenStorageModule = Modules.ContainsByPredicate([](const FShipModuleConfig& Module)
	{
		const UShipModuleDefinition* Definition = Module.ModuleDefinition.LoadSynchronous();
		return Definition && Definition->HasCapability(EShipModuleCapability::OxygenStorage);
	});
	const bool bHasFuelStorageModule = Modules.ContainsByPredicate([](const FShipModuleConfig& Module)
	{
		const UShipModuleDefinition* Definition = Module.ModuleDefinition.LoadSynchronous();
		return Definition && Definition->HasCapability(EShipModuleCapability::FuelStorage);
	});

	if (!bHasReactor)
	{
		OutErrors.Add(FText::FromString(TEXT("Missing mandatory module type: Reactor.")));
	}
	if (!bHasEngine)
	{
		OutErrors.Add(FText::FromString(TEXT("Missing mandatory module type: Engine.")));
	}
	if (!bHasBridge)
	{
		OutErrors.Add(FText::FromString(TEXT("Missing mandatory module type: Bridge.")));
	}
	if (!bHasAirlock)
	{
		OutErrors.Add(FText::FromString(TEXT("Missing mandatory module type: Airlock.")));
	}
	if (!bHasOxygenStorageModule)
	{
		OutErrors.Add(FText::FromString(TEXT("Missing mandatory module capability: OxygenStorage.")));
	}
	if (!bHasFuelStorageModule)
	{
		OutErrors.Add(FText::FromString(TEXT("Missing mandatory module capability: FuelStorage.")));
	}

	TMap<FName, int32> ModuleIndexById;
	for (int32 i = 0; i < Modules.Num(); ++i)
	{
		FName EffectiveId = Modules[i].ModuleId;
		if (EffectiveId.IsNone())
		{
			if (const UShipModuleDefinition* Definition = Modules[i].ModuleDefinition.LoadSynchronous())
			{
				EffectiveId = Definition->ModuleDefinitionId;
			}
		}
		if (EffectiveId.IsNone())
		{
			OutErrors.Add(FText::FromString(TEXT("Every module must have non-empty ModuleId.")));
			continue;
		}
		if (ModuleIndexById.Contains(EffectiveId))
		{
			OutErrors.Add(FText::Format(FText::FromString(TEXT("Duplicate ModuleId: {0}")), FText::FromName(EffectiveId)));
			continue;
		}
		ModuleIndexById.Add(EffectiveId, i);
	}

	for (const FShipConnectionConfig& Connection : Connections)
	{
		if (!ModuleIndexById.Contains(Connection.ModuleA) || !ModuleIndexById.Contains(Connection.ModuleB))
		{
			OutErrors.Add(FText::Format(
				FText::FromString(TEXT("Connection references unknown module: {0} <-> {1}")),
				FText::FromName(Connection.ModuleA),
				FText::FromName(Connection.ModuleB)));
		}
	}

	int32 ReactorIndex = INDEX_NONE;
	int32 BridgeIndex = INDEX_NONE;
	for (int32 i = 0; i < Modules.Num(); ++i)
	{
		const UShipModuleDefinition* Definition = Modules[i].ModuleDefinition.LoadSynchronous();
		const bool bReactor = Definition
			? Definition->HasCapability(EShipModuleCapability::ReactorCore)
			: (Modules[i].ModuleType == EShipModuleType::Reactor);
		const bool bBridge = Definition
			? Definition->HasCapability(EShipModuleCapability::BridgeCore)
			: (Modules[i].ModuleType == EShipModuleType::Bridge);
		if (bReactor && ReactorIndex == INDEX_NONE)
		{
			ReactorIndex = i;
		}
		if (bBridge && BridgeIndex == INDEX_NONE)
		{
			BridgeIndex = i;
		}
	}

	if (ReactorIndex != INDEX_NONE && BridgeIndex != INDEX_NONE)
	{
		TArray<TArray<int32>> Graph;
		Graph.SetNum(Modules.Num());
		for (const FShipConnectionConfig& Connection : Connections)
		{
			const int32* A = ModuleIndexById.Find(Connection.ModuleA);
			const int32* B = ModuleIndexById.Find(Connection.ModuleB);
			if (!A || !B)
			{
				continue;
			}
			Graph[*A].Add(*B);
			Graph[*B].Add(*A);
		}

		TArray<bool> Visited;
		Visited.Init(false, Modules.Num());
		TQueue<int32> Queue;
		Queue.Enqueue(ReactorIndex);
		Visited[ReactorIndex] = true;

		bool bConnected = false;
		while (!Queue.IsEmpty())
		{
			int32 Current = INDEX_NONE;
			Queue.Dequeue(Current);
			if (Current == BridgeIndex)
			{
				bConnected = true;
				break;
			}

			for (int32 Next : Graph[Current])
			{
				if (!Visited[Next])
				{
					Visited[Next] = true;
					Queue.Enqueue(Next);
				}
			}
		}

		if (!bConnected)
		{
			OutErrors.Add(FText::FromString(TEXT("Reactor and Bridge are disconnected.")));
		}
	}

	return OutErrors.Num() == 0;
}

