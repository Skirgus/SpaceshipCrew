// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShipConfigEditorLibrary.h"

#if WITH_EDITOR
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
#endif

bool UShipConfigEditorLibrary::AddOrUpdateModule(UShipConfigAsset* Config, const FShipModuleConfig& Module)
{
	if (!Config || Module.ModuleId.IsNone())
	{
		return false;
	}

	const int32 ExistingIndex = Config->Modules.IndexOfByPredicate([&Module](const FShipModuleConfig& Item)
	{
		return Item.ModuleId == Module.ModuleId;
	});
	if (ExistingIndex != INDEX_NONE)
	{
		Config->Modules[ExistingIndex] = Module;
	}
	else
	{
		Config->Modules.Add(Module);
	}

	Config->MarkPackageDirty();
	return true;
}

bool UShipConfigEditorLibrary::RemoveModule(UShipConfigAsset* Config, FName ModuleId)
{
	if (!Config || ModuleId.IsNone())
	{
		return false;
	}

	const int32 RemovedModules = Config->Modules.RemoveAll([ModuleId](const FShipModuleConfig& Item)
	{
		return Item.ModuleId == ModuleId;
	});
	if (RemovedModules <= 0)
	{
		return false;
	}

	Config->Connections.RemoveAll([ModuleId](const FShipConnectionConfig& Connection)
	{
		return Connection.ModuleA == ModuleId || Connection.ModuleB == ModuleId;
	});
	Config->MarkPackageDirty();
	return true;
}

bool UShipConfigEditorLibrary::AddConnection(UShipConfigAsset* Config, const FShipConnectionConfig& Connection)
{
	if (!Config || Connection.ModuleA.IsNone() || Connection.ModuleB.IsNone() || Connection.ModuleA == Connection.ModuleB)
	{
		return false;
	}

	const bool bExists = Config->Connections.ContainsByPredicate([&Connection](const FShipConnectionConfig& Item)
	{
		const bool bSameDirection = Item.ModuleA == Connection.ModuleA && Item.ModuleB == Connection.ModuleB;
		const bool bOppositeDirection = Item.ModuleA == Connection.ModuleB && Item.ModuleB == Connection.ModuleA;
		return bSameDirection || bOppositeDirection;
	});

	if (!bExists)
	{
		Config->Connections.Add(Connection);
		Config->MarkPackageDirty();
	}
	return true;
}

bool UShipConfigEditorLibrary::SetConnectionOpenByDefault(UShipConfigAsset* Config, FName ModuleA, FName ModuleB, bool bOpenByDefault)
{
	if (!Config)
	{
		return false;
	}

	for (FShipConnectionConfig& Connection : Config->Connections)
	{
		const bool bSameDirection = Connection.ModuleA == ModuleA && Connection.ModuleB == ModuleB;
		const bool bOppositeDirection = Connection.ModuleA == ModuleB && Connection.ModuleB == ModuleA;
		if (bSameDirection || bOppositeDirection)
		{
			Connection.bOpenByDefault = bOpenByDefault;
			Config->MarkPackageDirty();
			return true;
		}
	}
	return false;
}

bool UShipConfigEditorLibrary::SaveConfigAsset(UShipConfigAsset* Config)
{
	if (!Config)
	{
		return false;
	}

#if WITH_EDITOR
	UPackage* Package = Config->GetOutermost();
	if (!Package)
	{
		return false;
	}

	Package->MarkPackageDirty();
	const FString PackageName = Package->GetName();
	const FString PackageFileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.Error = GError;
	return UPackage::SavePackage(Package, Config, *PackageFileName, SaveArgs);
#else
	return false;
#endif
}

