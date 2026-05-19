#include "ShipBlueprintRegistry.h"

#include "ShipBlueprintDefinition.h"
#include "ShipBlueprintNaming.h"
#include "ShipBlueprintSerializer.h"
#include "ShipModuleCatalog.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "UObject/UObjectGlobals.h"

DEFINE_LOG_CATEGORY_STATIC(LogShipBlueprintRegistry, Log, All);

namespace ShipBlueprintRegistryPrivate
{
	static void ApplyPlayReadyToEntry(
		FShipBlueprintListEntry& Entry,
		const FShipBlueprintDocument& Document,
		UShipBlueprintRegistry* Registry)
	{
		Entry.bPlayReady = false;
		if (!Registry)
		{
			return;
		}
		UGameInstance* GI = Registry->GetGameInstance();
		UShipModuleCatalog* Catalog = GI ? GI->GetSubsystem<UShipModuleCatalog>() : nullptr;
		if (!Catalog)
		{
			return;
		}
		TArray<FString> PlayBlockers;
		Entry.bPlayReady = FShipBlueprintSerializer::ValidateDocumentForPlay(Document, *Catalog, PlayBlockers);
	}
}

FString UShipBlueprintRegistry::GetPlayerBlueprintsDirectory()
{
	return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("ShipBlueprints"));
}

FString UShipBlueprintRegistry::GetBundledProjectJsonDirectory()
{
	return FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Data/Ships"));
}

void UShipBlueprintRegistry::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	RefreshAll();
}

void UShipBlueprintRegistry::Deinitialize()
{
	LoadedProjectAssets.Empty();
	ProjectEntries.Empty();
	PlayerEntries.Empty();
	BundledProjectJsonPaths.Empty();
	bProjectCatalogCached = false;
	bPlayerCatalogCached = false;
	Super::Deinitialize();
}

void UShipBlueprintRegistry::RefreshAll()
{
	bProjectCatalogCached = false;
	bPlayerCatalogCached = false;
	RefreshProjectCatalog();
	RefreshPlayerCatalog();
}

void UShipBlueprintRegistry::RefreshProjectCatalog()
{
	LoadedProjectAssets.Empty();
	BundledProjectJsonPaths.Empty();
	ProjectEntries.Empty();
	bProjectCatalogCached = false;

	ScanProjectAssets();
	ScanBundledProjectJson();

	bProjectCatalogCached = true;
}

void UShipBlueprintRegistry::RefreshPlayerCatalog()
{
	ScanPlayerJson();
	bPlayerCatalogCached = true;
}

void UShipBlueprintRegistry::EnsureProjectCatalogFresh()
{
	if (!bProjectCatalogCached)
	{
		RefreshProjectCatalog();
	}
}

void UShipBlueprintRegistry::EnsurePlayerCatalogFresh()
{
	if (!bPlayerCatalogCached)
	{
		RefreshPlayerCatalog();
	}
}

void UShipBlueprintRegistry::ScanProjectAssets()
{
	FAssetRegistryModule& AssetRegistryModule =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

#if WITH_EDITOR
	{
		TArray<FString> PathsToScan;
		PathsToScan.Add(TEXT("/Game/Data/Ships"));
		AssetRegistry.ScanPathsSynchronous(PathsToScan, true);
	}
#endif

	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.bRecursiveClasses = true;
	Filter.PackagePaths.Add(FName(TEXT("/Game/Data/Ships")));
	Filter.ClassPaths.Add(FTopLevelAssetPath(UShipBlueprintDefinition::StaticClass()));

	TArray<FAssetData> AssetList;
	AssetRegistry.GetAssets(Filter, AssetList);

	for (const FAssetData& AssetData : AssetList)
	{
		if (!AssetData.IsValid() || AssetData.IsRedirector())
		{
			continue;
		}

		UShipBlueprintDefinition* Def = Cast<UShipBlueprintDefinition>(AssetData.GetAsset());
		if (!Def)
		{
			Def = Cast<UShipBlueprintDefinition>(
				StaticLoadObject(UShipBlueprintDefinition::StaticClass(), nullptr,
					*AssetData.GetSoftObjectPath().ToString()));
		}
		if (!Def || Def->ShipId.IsNone())
		{
			continue;
		}

		LoadedProjectAssets.Add(Def);

		FShipBlueprintListEntry Entry;
		Entry.Source = EShipBlueprintSource::Project;
		Entry.ShipId = Def->ShipId;
		Entry.DisplayName = Def->DisplayName.IsEmpty() ? FText::FromName(Def->ShipId) : Def->DisplayName;
		ShipBlueprintRegistryPrivate::ApplyPlayReadyToEntry(Entry, Def->BuildDocument(), this);
		ProjectEntries.Add(Entry);
	}

	ProjectEntries.Sort([](const FShipBlueprintListEntry& A, const FShipBlueprintListEntry& B)
	{
		return A.ShipId.LexicalLess(B.ShipId);
	});
}

void UShipBlueprintRegistry::ScanBundledProjectJson()
{
	const FString Dir = GetBundledProjectJsonDirectory();
	IFileManager& FM = IFileManager::Get();
	if (!FM.DirectoryExists(*Dir))
	{
		UE_LOG(LogShipBlueprintRegistry, Verbose,
			TEXT("ScanBundledProjectJson: каталог не найден: %s"), *Dir);
		return;
	}

	TArray<FString> JsonFiles;
	FM.FindFiles(JsonFiles, *Dir, TEXT("json"));

	for (const FString& FileName : JsonFiles)
	{
		const FString FullPath = FPaths::Combine(Dir, FileName);
		FShipBlueprintDocument Doc;
		FString Error;
		if (!LoadJsonFile(FullPath, Doc, Error))
		{
			UE_LOG(LogShipBlueprintRegistry, Warning, TEXT("ShipBlueprintRegistry: %s — %s"), *FullPath, *Error);
			continue;
		}
		if (Doc.ShipId.IsNone())
		{
			Doc.ShipId = FName(*FPaths::GetBaseFilename(FileName));
		}

		const bool bAlreadyListed = ProjectEntries.ContainsByPredicate([&Doc](const FShipBlueprintListEntry& E)
		{
			return E.ShipId == Doc.ShipId;
		});
		if (bAlreadyListed)
		{
			continue;
		}

		BundledProjectJsonPaths.Add(Doc.ShipId, FullPath);

		FShipBlueprintListEntry Entry;
		Entry.Source = EShipBlueprintSource::Project;
		Entry.ShipId = Doc.ShipId;
		Entry.DisplayName = Doc.DisplayName.IsEmpty() ? FText::FromName(Doc.ShipId) : Doc.DisplayName;
		ShipBlueprintRegistryPrivate::ApplyPlayReadyToEntry(Entry, Doc, this);
		ProjectEntries.Add(Entry);
	}

	ProjectEntries.Sort([](const FShipBlueprintListEntry& A, const FShipBlueprintListEntry& B)
	{
		return A.ShipId.LexicalLess(B.ShipId);
	});
}

void UShipBlueprintRegistry::ScanPlayerJson()
{
	PlayerEntries.Reset();

	const FString Dir = GetPlayerBlueprintsDirectory();
	IFileManager& FM = IFileManager::Get();
	FM.MakeDirectory(*Dir, true);

	TArray<FString> JsonFiles;
	FM.FindFiles(JsonFiles, *Dir, TEXT("json"));

	for (const FString& FileName : JsonFiles)
	{
		const FString FullPath = FPaths::Combine(Dir, FileName);
		FShipBlueprintDocument Doc;
		FString Error;
		if (!LoadJsonFile(FullPath, Doc, Error))
		{
			UE_LOG(LogShipBlueprintRegistry, Warning, TEXT("ShipBlueprintRegistry player: %s — %s"), *FullPath, *Error);
			continue;
		}
		if (Doc.ShipId.IsNone())
		{
			Doc.ShipId = FName(*FPaths::GetBaseFilename(FileName));
		}

		FShipBlueprintListEntry Entry;
		Entry.Source = EShipBlueprintSource::Player;
		Entry.ShipId = Doc.ShipId;
		Entry.DisplayName = Doc.DisplayName.IsEmpty() ? FText::FromName(Doc.ShipId) : Doc.DisplayName;
		ShipBlueprintRegistryPrivate::ApplyPlayReadyToEntry(Entry, Doc, this);
		PlayerEntries.Add(Entry);
	}

	PlayerEntries.Sort([](const FShipBlueprintListEntry& A, const FShipBlueprintListEntry& B)
	{
		return A.ShipId.LexicalLess(B.ShipId);
	});
}

bool UShipBlueprintRegistry::LoadDocument(
	const EShipBlueprintSource Source,
	const FName ShipId,
	FShipBlueprintDocument& OutDocument,
	FString& OutError) const
{
	if (ShipId.IsNone())
	{
		OutError = TEXT("Пустой ShipId.");
		return false;
	}

	if (Source == EShipBlueprintSource::Project)
	{
		for (const TObjectPtr<UShipBlueprintDefinition>& Def : LoadedProjectAssets)
		{
			if (Def && Def->ShipId == ShipId)
			{
				OutDocument = Def->BuildDocument();
				return true;
			}
		}

		if (const FString* JsonPath = BundledProjectJsonPaths.Find(ShipId))
		{
			return LoadJsonFile(*JsonPath, OutDocument, OutError);
		}

		OutError = FString::Printf(TEXT("Проектный чертёж не найден: %s"), *ShipId.ToString());
		return false;
	}

	FString PlayerPath;
	if (!TryMakePlayerFilePath(ShipId, PlayerPath, OutError))
	{
		return false;
	}
	if (!FPaths::FileExists(PlayerPath))
	{
		OutError = FString::Printf(TEXT("Сохранение игрока не найдено: %s"), *ShipId.ToString());
		return false;
	}
	return LoadJsonFile(PlayerPath, OutDocument, OutError);
}

bool UShipBlueprintRegistry::SavePlayerBlueprint(const FShipBlueprintDocument& Document, FString& OutError)
{
	if (Document.ShipId.IsNone())
	{
		OutError = TEXT("Нельзя сохранить корабль без ShipId.");
		return false;
	}

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UShipModuleCatalog* Catalog = GI->GetSubsystem<UShipModuleCatalog>())
		{
			if (!FShipBlueprintSerializer::ValidateDocumentForSave(Document, *Catalog, OutError))
			{
				return false;
			}
		}
		else
		{
			OutError = TEXT("Каталог модулей недоступен.");
			return false;
		}
	}
	else
	{
		OutError = TEXT("GameInstance недоступен.");
		return false;
	}

	FString PlayerPath;
	if (!TryMakePlayerFilePath(Document.ShipId, PlayerPath, OutError))
	{
		return false;
	}

	const FString Dir = GetPlayerBlueprintsDirectory();
	IFileManager::Get().MakeDirectory(*Dir, true);

	FShipBlueprintDocument ToSave = Document;
	ToSave.SchemaVersion = FShipBlueprintDocument::CurrentSchemaVersion;

	if (!WriteJsonFile(PlayerPath, ToSave, OutError))
	{
		return false;
	}

	RefreshPlayerCatalog();
	return true;
}

bool UShipBlueprintRegistry::DoesPlayerBlueprintExist(const FName ShipId) const
{
	FString PlayerPath;
	FString PathError;
	if (!TryMakePlayerFilePath(ShipId, PlayerPath, PathError))
	{
		return false;
	}
	return FPaths::FileExists(PlayerPath);
}

bool UShipBlueprintRegistry::DeletePlayerBlueprint(const FName ShipId, FString& OutError)
{
	FString Path;
	if (!TryMakePlayerFilePath(ShipId, Path, OutError))
	{
		return false;
	}
	if (!FPaths::FileExists(Path))
	{
		OutError = TEXT("Файл не найден.");
		return false;
	}
	if (!IFileManager::Get().Delete(*Path))
	{
		OutError = TEXT("Не удалось удалить файл.");
		return false;
	}
	RefreshPlayerCatalog();
	return true;
}

bool UShipBlueprintRegistry::LoadJsonFile(const FString& FilePath, FShipBlueprintDocument& OutDocument, FString& OutError) const
{
	FString Json;
	if (!FFileHelper::LoadFileToString(Json, *FilePath))
	{
		OutError = FString::Printf(TEXT("Не удалось прочитать: %s"), *FilePath);
		return false;
	}
	return FShipBlueprintSerializer::JsonStringToDocument(Json, OutDocument, OutError);
}

bool UShipBlueprintRegistry::WriteJsonFile(const FString& FilePath, const FShipBlueprintDocument& Document, FString& OutError) const
{
	FString Json;
	if (!FShipBlueprintSerializer::DocumentToJsonString(Document, Json))
	{
		OutError = TEXT("Сериализация в JSON не удалась.");
		return false;
	}
	if (!FFileHelper::SaveStringToFile(Json, *FilePath))
	{
		OutError = FString::Printf(TEXT("Не удалось записать: %s"), *FilePath);
		return false;
	}
	return true;
}

bool UShipBlueprintRegistry::TryMakePlayerFilePath(const FName ShipId, FString& OutPath, FString& OutError) const
{
	FString FileBaseName;
	if (!ShipBlueprintNaming::TrySanitizeShipIdForFilename(ShipId, FileBaseName))
	{
		OutError = FString::Printf(
			TEXT("Недопустимый идентификатор корабля для файла: %s"),
			*ShipId.ToString());
		return false;
	}

	const FString Dir = GetPlayerBlueprintsDirectory();
	const FString Combined = FPaths::Combine(Dir, FileBaseName + TEXT(".json"));
	const FString FullPath = FPaths::ConvertRelativePathToFull(Combined);
	const FString FullDir = FPaths::ConvertRelativePathToFull(Dir);

	if (!FullPath.StartsWith(FullDir))
	{
		OutError = TEXT("Путь сохранения выходит за пределы каталога ShipBlueprints.");
		return false;
	}

	OutPath = FullPath;
	return true;
}
