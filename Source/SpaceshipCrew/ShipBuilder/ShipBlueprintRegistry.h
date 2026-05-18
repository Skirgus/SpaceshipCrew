#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ShipBuilder/ShipBlueprintTypes.h"
#include "ShipBlueprintRegistry.generated.h"

class UShipBlueprintDefinition;
class UShipModuleCatalog;

/**
 * Каталог чертежей корабля: шаблоны проекта (Data Asset + JSON в Content) и сохранения игрока.
 */
UCLASS()
class SPACESHIPCREW_API UShipBlueprintRegistry : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Полное обновление (инициализация, явный сброс кэша). */
	UFUNCTION(BlueprintCallable, Category = "ShipBlueprint")
	void RefreshAll();

	/** Обновить только проектные шаблоны (ассеты + bundled JSON). */
	UFUNCTION(BlueprintCallable, Category = "ShipBlueprint")
	void RefreshProjectCatalog();

	/** Обновить только список сохранений игрока. */
	UFUNCTION(BlueprintCallable, Category = "ShipBlueprint")
	void RefreshPlayerCatalog();

	/** Использовать кэш проектных шаблонов; при необходимости — один раз сканировать. */
	void EnsureProjectCatalogFresh();

	/** Использовать кэш сохранений игрока; после Save/Delete — RefreshPlayerCatalog. */
	void EnsurePlayerCatalogFresh();

	UFUNCTION(BlueprintCallable, Category = "ShipBlueprint")
	TArray<FShipBlueprintListEntry> GetProjectBlueprints() const { return ProjectEntries; }

	UFUNCTION(BlueprintCallable, Category = "ShipBlueprint")
	TArray<FShipBlueprintListEntry> GetPlayerBlueprints() const { return PlayerEntries; }

	UFUNCTION(BlueprintCallable, Category = "ShipBlueprint")
	bool LoadDocument(EShipBlueprintSource Source, FName ShipId, FShipBlueprintDocument& OutDocument, FString& OutError) const;

	UFUNCTION(BlueprintCallable, Category = "ShipBlueprint")
	bool SavePlayerBlueprint(const FShipBlueprintDocument& Document, FString& OutError);

	UFUNCTION(BlueprintCallable, Category = "ShipBlueprint")
	bool DeletePlayerBlueprint(FName ShipId, FString& OutError);

	UFUNCTION(BlueprintCallable, Category = "ShipBlueprint")
	bool DoesPlayerBlueprintExist(FName ShipId) const;

	/** Каталог Saved/ShipBlueprints. */
	static FString GetPlayerBlueprintsDirectory();

	/** Каталог Content/Data/Ships для встроенных JSON-шаблонов (editor и staged Non-UFS). */
	static FString GetBundledProjectJsonDirectory();

private:
	void ScanProjectAssets();
	void ScanBundledProjectJson();
	void ScanPlayerJson();

	bool LoadJsonFile(const FString& FilePath, FShipBlueprintDocument& OutDocument, FString& OutError) const;
	bool WriteJsonFile(const FString& FilePath, const FShipBlueprintDocument& Document, FString& OutError) const;
	bool TryMakePlayerFilePath(FName ShipId, FString& OutPath, FString& OutError) const;

	UPROPERTY()
	TArray<TObjectPtr<UShipBlueprintDefinition>> LoadedProjectAssets;

	TArray<FShipBlueprintListEntry> ProjectEntries;
	TArray<FShipBlueprintListEntry> PlayerEntries;

	/** ShipId -> путь к JSON в Content (если не Data Asset). */
	TMap<FName, FString> BundledProjectJsonPaths;

	bool bProjectCatalogCached = false;
	bool bPlayerCatalogCached = false;
};
