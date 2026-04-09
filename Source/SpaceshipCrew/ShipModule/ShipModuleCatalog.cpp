#include "ShipModuleCatalog.h"
#include "ShipModuleDefinition.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "HAL/IConsoleManager.h"
#include "Modules/ModuleManager.h"
#include "UObject/UObjectGlobals.h"

DEFINE_LOG_CATEGORY_STATIC(LogShipModuleCatalog, Log, All);

// ----------------------------------------------------------------------------
// Консольная команда
// ----------------------------------------------------------------------------

static FAutoConsoleCommandWithWorldAndArgs GShipModuleListAllCmd(
	TEXT("ShipModule.ListAll"),
	TEXT("Выводит в лог все модули из каталога ShipModuleCatalog."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&UShipModuleCatalog::ListAllModulesCommand));

void UShipModuleCatalog::ListAllModulesCommand(const TArray<FString>& Args, UWorld* World)
{
	if (!World)
	{
		UE_LOG(LogShipModuleCatalog, Warning, TEXT("ShipModule.ListAll: World недоступен."));
		return;
	}

	const UGameInstance* GI = World->GetGameInstance();
	if (!GI)
	{
		UE_LOG(LogShipModuleCatalog, Warning, TEXT("ShipModule.ListAll: GameInstance недоступен."));
		return;
	}

	const UShipModuleCatalog* Catalog = GI->GetSubsystem<UShipModuleCatalog>();
	if (!Catalog)
	{
		UE_LOG(LogShipModuleCatalog, Warning, TEXT("ShipModule.ListAll: UShipModuleCatalog не найден."));
		return;
	}

	UE_LOG(LogShipModuleCatalog, Log, TEXT("=== Каталог модулей корабля: %d записей ==="), Catalog->GetModuleCount());

	for (const UShipModuleDefinition* Def : Catalog->GetAllModules())
	{
		if (!Def)
		{
			continue;
		}
		UE_LOG(LogShipModuleCatalog, Log,
			TEXT("  [%s] %s | Тип=%d | Масса=%.1f | Размер=(%.0f, %.0f, %.0f) | Интерьер=%s | КТ=%d"),
			*Def->ModuleId.ToString(),
			*Def->DisplayName.ToString(),
			static_cast<int32>(Def->ModuleType),
			Def->Mass,
			Def->Size.X, Def->Size.Y, Def->Size.Z,
			Def->bHasInterior ? TEXT("Да") : TEXT("Нет"),
			Def->ContactPoints.Num());
	}

	UE_LOG(LogShipModuleCatalog, Log, TEXT("=== Конец каталога ==="));
}

// ----------------------------------------------------------------------------
// Жизненный цикл подсистемы
// ----------------------------------------------------------------------------

void UShipModuleCatalog::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	ScanAndLoadModules();
}

void UShipModuleCatalog::Deinitialize()
{
	LoadedModules.Empty();
	Super::Deinitialize();
}

// ----------------------------------------------------------------------------
// Сканирование и загрузка
// ----------------------------------------------------------------------------

void UShipModuleCatalog::ScanAndLoadModules()
{
	LoadedModules.Empty();

	FAssetRegistryModule& AssetRegistryModule =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

#if WITH_EDITOR
	{
		TArray<FString> PathsToScan;
		PathsToScan.Add(TEXT("/Game"));
		AssetRegistry.ScanPathsSynchronous(PathsToScan, true);
	}
#endif

	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.bRecursiveClasses = true;
	Filter.PackagePaths.Add(FName(TEXT("/Game")));
	Filter.ClassPaths.Add(FTopLevelAssetPath(UShipModuleDefinition::StaticClass()));

	TArray<FAssetData> AssetList;
	AssetRegistry.GetAssets(Filter, AssetList);

	if (AssetList.Num() == 0)
	{
		UE_LOG(LogShipModuleCatalog, Log,
			TEXT("ShipModuleCatalog: в Asset Registry не найдено ассетов класса ShipModuleDefinition под /Game. "
				 "Создайте ассеты Ship Module Definition (Miscellaneous) и сохраните."));
		return;
	}

	for (const FAssetData& AssetData : AssetList)
	{
		if (!AssetData.IsValid() || AssetData.IsRedirector())
		{
			continue;
		}

		UShipModuleDefinition* Def = nullptr;
		if (UObject* Obj = AssetData.GetAsset())
		{
			Def = Cast<UShipModuleDefinition>(Obj);
		}
		if (!Def)
		{
			const FString PathString = AssetData.GetSoftObjectPath().ToString();
			Def = Cast<UShipModuleDefinition>(
				StaticLoadObject(UShipModuleDefinition::StaticClass(), nullptr, *PathString));
		}

		if (Def)
		{
			LoadedModules.Add(Def);
			UE_LOG(LogShipModuleCatalog, Verbose, TEXT("ShipModuleCatalog: + %s"),
				*AssetData.GetSoftObjectPath().ToString());
		}
		else
		{
			UE_LOG(LogShipModuleCatalog, Warning,
				TEXT("ShipModuleCatalog: не удалось загрузить %s"),
				*AssetData.GetSoftObjectPath().ToString());
		}
	}

	UE_LOG(LogShipModuleCatalog, Log,
		TEXT("ShipModuleCatalog: загружено %d модулей (поиск по Asset Registry, класс ShipModuleDefinition)."),
		LoadedModules.Num());
}

// ----------------------------------------------------------------------------
// Публичный API
// ----------------------------------------------------------------------------

TArray<UShipModuleDefinition*> UShipModuleCatalog::GetAllModules() const
{
	TArray<UShipModuleDefinition*> Result;
	Result.Reserve(LoadedModules.Num());
	for (const TObjectPtr<UShipModuleDefinition>& Def : LoadedModules)
	{
		if (Def)
		{
			Result.Add(Def);
		}
	}
	return Result;
}

UShipModuleDefinition* UShipModuleCatalog::FindModuleById(FName ModuleId) const
{
	for (const TObjectPtr<UShipModuleDefinition>& Def : LoadedModules)
	{
		if (Def && Def->ModuleId == ModuleId)
		{
			return Def;
		}
	}
	return nullptr;
}

int32 UShipModuleCatalog::GetModuleCount() const
{
	return LoadedModules.Num();
}
