#pragma once

#include "CoreMinimal.h"

class UShipModuleCatalog;
class UShipModuleDefinition;

/**
 * Экземпляр модуля в текущей сборке корабля.
 *
 * InstanceId — стабильный идентификатор узла в пределах одной конфигурации.
 * ModuleId ссылается на запись из каталога модулей T02a.
 */
struct FShipBuildModuleInstance
{
	FName InstanceId = NAME_None;
	FName ModuleId = NAME_None;
};

/**
 * Стыковка двух модулей через конкретные сокеты.
 *
 * Запись неориентированная по смыслу, но хранится как "A-B" для простоты API.
 */
struct FShipBuildModuleConnection
{
	FName ModuleAInstanceId = NAME_None;
	FName ModuleASocketName = NAME_None;
	FName ModuleBInstanceId = NAME_None;
	FName ModuleBSocketName = NAME_None;
};

/**
 * Результат валидации текущей конфигурации сборки.
 */
struct FShipBuildValidationResult
{
	bool bIsValid = false;
	TArray<FString> Errors;
	float TotalMass = 0.0f;
};

/**
 * Абстракция источника определений модулей.
 *
 * Доменная логика сборки не зависит от UI/сцены и использует только этот интерфейс
 * для получения данных модуля по ModuleId.
 */
class IShipBuildModuleResolver
{
public:
	virtual ~IShipBuildModuleResolver() = default;
	virtual const UShipModuleDefinition* ResolveModule(FName ModuleId) const = 0;
};

/**
 * Resolver, читающий определения модулей из UShipModuleCatalog (T02a).
 */
class FCatalogShipBuildModuleResolver final : public IShipBuildModuleResolver
{
public:
	explicit FCatalogShipBuildModuleResolver(const UShipModuleCatalog& InCatalog);

	virtual const UShipModuleDefinition* ResolveModule(FName ModuleId) const override;

private:
	const UShipModuleCatalog& Catalog;
};

/**
 * Доменная модель текущей сборки корабля (T02b).
 *
 * Предоставляет API для добавления/удаления/замены модулей, а также запросов
 * суммарной массы и валидации совместимости сокетов/контактных точек.
 */
class FShipBuildDomainModel
{
public:
	explicit FShipBuildDomainModel(const IShipBuildModuleResolver& InResolver);

	/** Добавляет первый (корневой) модуль в конфигурацию. */
	bool AddRootModule(FName InstanceId, FName ModuleId, FString* OutError = nullptr);

	/**
	 * Добавляет модуль и соединяет его с уже существующим модулем через сокеты.
	 */
	bool AddAttachedModule(
		FName NewInstanceId,
		FName NewModuleId,
		FName ExistingInstanceId,
		FName NewModuleSocketName,
		FName ExistingModuleSocketName,
		FString* OutError = nullptr);

	/** Удаляет модуль и все его соединения. */
	bool RemoveModule(FName InstanceId, FString* OutError = nullptr);

	/** Заменяет тип модуля у существующего узла (сохраняет все соединения узла). */
	bool ReplaceModule(FName InstanceId, FName NewModuleId, FString* OutError = nullptr);

	/** Детерминированный расчёт суммарной массы по текущей конфигурации. */
	float GetTotalMass() const;

	/** Полная валидация конфигурации сборки. */
	FShipBuildValidationResult Validate() const;

	const TArray<FShipBuildModuleInstance>& GetModules() const { return Modules; }
	const TArray<FShipBuildModuleConnection>& GetConnections() const { return Connections; }

private:
	const FShipBuildModuleInstance* FindModuleInstance(FName InstanceId) const;
	bool AreModuleTypesCompatible(const UShipModuleDefinition& Source, const UShipModuleDefinition& Target) const;
	void AddError(TArray<FString>& OutErrors, const FString& ErrorText) const;

	const IShipBuildModuleResolver& Resolver;
	TArray<FShipBuildModuleInstance> Modules;
	TArray<FShipBuildModuleConnection> Connections;
};
