#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ShipModuleCatalog.generated.h"

class UShipModuleDefinition;

/**
 * Каталог модулей корабля — единый рантайм-источник данных о доступных модулях.
 *
 * При инициализации сканирует AssetManager и загружает все ассеты типа "ShipModule".
 * Предоставляет API для конструктора (T02c), домена сборки (T02b), системы пресетов (T03) и отладки.
 * Консольная команда ShipModule.ListAll выводит содержимое каталога в лог.
 */
UCLASS()
class SPACESHIPCREW_API UShipModuleCatalog : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Все загруженные определения модулей. */
	UFUNCTION(BlueprintCallable, Category = "ShipModule")
	TArray<UShipModuleDefinition*> GetAllModules() const;

	/** Поиск модуля по ModuleId. Возвращает nullptr если не найден. */
	UFUNCTION(BlueprintCallable, Category = "ShipModule")
	UShipModuleDefinition* FindModuleById(FName ModuleId) const;

	/** Количество модулей в каталоге. */
	UFUNCTION(BlueprintCallable, Category = "ShipModule")
	int32 GetModuleCount() const;

private:
	void ScanAndLoadModules();
	void OnAssetManagerScanComplete();

	UPROPERTY()
	TArray<TObjectPtr<UShipModuleDefinition>> LoadedModules;

	static void ListAllModulesCommand(const TArray<FString>& Args, UWorld* World);
};
