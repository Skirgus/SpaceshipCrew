#include "ShipBuildDomain.h"

#include "ShipModuleCatalog.h"
#include "ShipModuleDefinition.h"
#include "ShipModuleTypes.h"

#include "Algo/Sort.h"
#include "HAL/IConsoleManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogShipBuildDomain, Log, All);

namespace ShipBuildDomainPrivate
{
	static void SetError(FString* OutError, const FString& ErrorText)
	{
		if (OutError)
		{
			*OutError = ErrorText;
		}
	}

	static bool AreSocketTypesCompatible(
		const EShipModuleSocketType SocketTypeA,
		const EShipModuleSocketType SocketTypeB)
	{
		return SocketTypeA == EShipModuleSocketType::Universal
			|| SocketTypeB == EShipModuleSocketType::Universal
			|| SocketTypeA == SocketTypeB;
	}

	static bool IsTypeAllowedBySource(
		const UShipModuleDefinition& Source,
		const EShipModuleType TargetType)
	{
		// Пустой список трактуем как отсутствие ограничений.
		return Source.CompatibleModuleTypes.Num() == 0
			|| Source.CompatibleModuleTypes.Contains(TargetType);
	}

	static const FShipModuleContactPoint* FindSocketByName(
		const UShipModuleDefinition& ModuleDefinition,
		const FName SocketName)
	{
		for (const FShipModuleContactPoint& ContactPoint : ModuleDefinition.ContactPoints)
		{
			if (ContactPoint.SocketName == SocketName)
			{
				return &ContactPoint;
			}
		}
		return nullptr;
	}

	static const UShipModuleDefinition* FindAnyDefinitionWithSocket(const TArray<UShipModuleDefinition*>& Definitions)
	{
		for (const UShipModuleDefinition* Definition : Definitions)
		{
			if (Definition && Definition->ContactPoints.Num() > 0)
			{
				return Definition;
			}
		}
		return nullptr;
	}

	static bool TryFindCompatiblePair(
		const TArray<UShipModuleDefinition*>& Definitions,
		const UShipModuleDefinition*& OutA,
		const FShipModuleContactPoint*& OutSocketA,
		const UShipModuleDefinition*& OutB,
		const FShipModuleContactPoint*& OutSocketB)
	{
		for (const UShipModuleDefinition* CandidateA : Definitions)
		{
			if (!CandidateA)
			{
				continue;
			}

			for (const UShipModuleDefinition* CandidateB : Definitions)
			{
				if (!CandidateB)
				{
					continue;
				}

				for (const FShipModuleContactPoint& SocketA : CandidateA->ContactPoints)
				{
					for (const FShipModuleContactPoint& SocketB : CandidateB->ContactPoints)
					{
						const bool bSocketCompatible = AreSocketTypesCompatible(SocketA.SocketType, SocketB.SocketType);
						const bool bTypeCompatibleAB = IsTypeAllowedBySource(*CandidateA, CandidateB->ModuleType);
						const bool bTypeCompatibleBA = IsTypeAllowedBySource(*CandidateB, CandidateA->ModuleType);
						if (bSocketCompatible && bTypeCompatibleAB && bTypeCompatibleBA)
						{
							OutA = CandidateA;
							OutSocketA = &SocketA;
							OutB = CandidateB;
							OutSocketB = &SocketB;
							return true;
						}
					}
				}
			}
		}

		return false;
	}
}

FCatalogShipBuildModuleResolver::FCatalogShipBuildModuleResolver(const UShipModuleCatalog& InCatalog)
	: Catalog(InCatalog)
{
}

const UShipModuleDefinition* FCatalogShipBuildModuleResolver::ResolveModule(const FName ModuleId) const
{
	return Catalog.FindModuleById(ModuleId);
}

FShipBuildDomainModel::FShipBuildDomainModel(const IShipBuildModuleResolver& InResolver)
	: Resolver(InResolver)
{
}

bool FShipBuildDomainModel::AddRootModule(FName InstanceId, FName ModuleId, FString* OutError)
{
	if (InstanceId.IsNone())
	{
		ShipBuildDomainPrivate::SetError(OutError, TEXT("InstanceId обязателен."));
		return false;
	}

	if (ModuleId.IsNone())
	{
		ShipBuildDomainPrivate::SetError(OutError, TEXT("ModuleId обязателен."));
		return false;
	}

	if (FindModuleInstance(InstanceId))
	{
		ShipBuildDomainPrivate::SetError(OutError, FString::Printf(TEXT("InstanceId '%s' уже используется."), *InstanceId.ToString()));
		return false;
	}

	if (Resolver.ResolveModule(ModuleId) == nullptr)
	{
		ShipBuildDomainPrivate::SetError(OutError, FString::Printf(TEXT("ModuleId '%s' не найден в каталоге."), *ModuleId.ToString()));
		return false;
	}

	Modules.Add(FShipBuildModuleInstance{ InstanceId, ModuleId });
	return true;
}

bool FShipBuildDomainModel::AddAttachedModule(
	FName NewInstanceId,
	FName NewModuleId,
	FName ExistingInstanceId,
	FName NewModuleSocketName,
	FName ExistingModuleSocketName,
	FString* OutError)
{
	if (ExistingInstanceId.IsNone() || NewModuleSocketName.IsNone() || ExistingModuleSocketName.IsNone())
	{
		ShipBuildDomainPrivate::SetError(OutError, TEXT("Для стыковки требуются ExistingInstanceId и оба SocketName."));
		return false;
	}

	if (FindModuleInstance(ExistingInstanceId) == nullptr)
	{
		ShipBuildDomainPrivate::SetError(OutError, FString::Printf(TEXT("Модуль '%s' не найден."), *ExistingInstanceId.ToString()));
		return false;
	}

	if (!AddRootModule(NewInstanceId, NewModuleId, OutError))
	{
		return false;
	}

	Connections.Add(FShipBuildModuleConnection{
		NewInstanceId,
		NewModuleSocketName,
		ExistingInstanceId,
		ExistingModuleSocketName
	});

	return true;
}

bool FShipBuildDomainModel::RemoveModule(const FName InstanceId, FString* OutError)
{
	const int32 ModuleIndex = Modules.IndexOfByPredicate([InstanceId](const FShipBuildModuleInstance& Module)
	{
		return Module.InstanceId == InstanceId;
	});
	if (ModuleIndex == INDEX_NONE)
	{
		ShipBuildDomainPrivate::SetError(OutError, FString::Printf(TEXT("Модуль '%s' не найден."), *InstanceId.ToString()));
		return false;
	}

	Modules.RemoveAt(ModuleIndex);
	Connections.RemoveAll([InstanceId](const FShipBuildModuleConnection& Connection)
	{
		return Connection.ModuleAInstanceId == InstanceId || Connection.ModuleBInstanceId == InstanceId;
	});
	return true;
}

bool FShipBuildDomainModel::ReplaceModule(FName InstanceId, FName NewModuleId, FString* OutError)
{
	FShipBuildModuleInstance* ExistingModule = Modules.FindByPredicate([InstanceId](const FShipBuildModuleInstance& Module)
	{
		return Module.InstanceId == InstanceId;
	});
	if (!ExistingModule)
	{
		ShipBuildDomainPrivate::SetError(OutError, FString::Printf(TEXT("Модуль '%s' не найден."), *InstanceId.ToString()));
		return false;
	}

	if (Resolver.ResolveModule(NewModuleId) == nullptr)
	{
		ShipBuildDomainPrivate::SetError(OutError, FString::Printf(TEXT("ModuleId '%s' не найден в каталоге."), *NewModuleId.ToString()));
		return false;
	}

	ExistingModule->ModuleId = NewModuleId;
	return true;
}

float FShipBuildDomainModel::GetTotalMass() const
{
	// Суммируем в стабильном порядке, чтобы убрать зависимость от порядка мутаций.
	TArray<FShipBuildModuleInstance> SortedModules = Modules;
	Algo::Sort(SortedModules, [](const FShipBuildModuleInstance& A, const FShipBuildModuleInstance& B)
	{
		return A.InstanceId.LexicalLess(B.InstanceId);
	});

	double TotalMass = 0.0;
	for (const FShipBuildModuleInstance& Module : SortedModules)
	{
		if (const UShipModuleDefinition* Definition = Resolver.ResolveModule(Module.ModuleId))
		{
			TotalMass += static_cast<double>(Definition->Mass);
		}
	}
	return static_cast<float>(TotalMass);
}

FShipBuildValidationResult FShipBuildDomainModel::Validate() const
{
	FShipBuildValidationResult Result;
	Result.TotalMass = GetTotalMass();
	Result.bIsValid = true;

	if (Modules.Num() == 0)
	{
		AddError(Result.Errors, TEXT("Конфигурация пустая: требуется хотя бы один модуль."));
	}

	// Кэшируем определения, чтобы проверять связи без повторных Resolve.
	TMap<FName, const UShipModuleDefinition*> DefinitionByInstance;
	for (const FShipBuildModuleInstance& Module : Modules)
	{
		if (Module.InstanceId.IsNone())
		{
			AddError(Result.Errors, TEXT("Обнаружен модуль с пустым InstanceId."));
			continue;
		}

		if (Module.ModuleId.IsNone())
		{
			AddError(Result.Errors, FString::Printf(TEXT("Instance '%s' содержит пустой ModuleId."), *Module.InstanceId.ToString()));
			continue;
		}

		const UShipModuleDefinition* Definition = Resolver.ResolveModule(Module.ModuleId);
		if (!Definition)
		{
			AddError(Result.Errors, FString::Printf(
				TEXT("ModuleId '%s' для instance '%s' отсутствует в каталоге."),
				*Module.ModuleId.ToString(),
				*Module.InstanceId.ToString()));
			continue;
		}

		DefinitionByInstance.Add(Module.InstanceId, Definition);
	}

	TMap<FString, int32> SocketUsageCount;

	for (const FShipBuildModuleConnection& Connection : Connections)
	{
		const UShipModuleDefinition* DefinitionA = DefinitionByInstance.FindRef(Connection.ModuleAInstanceId);
		const UShipModuleDefinition* DefinitionB = DefinitionByInstance.FindRef(Connection.ModuleBInstanceId);

		if (!DefinitionA)
		{
			AddError(Result.Errors, FString::Printf(TEXT("Связь ссылается на отсутствующий module instance '%s'."), *Connection.ModuleAInstanceId.ToString()));
			continue;
		}
		if (!DefinitionB)
		{
			AddError(Result.Errors, FString::Printf(TEXT("Связь ссылается на отсутствующий module instance '%s'."), *Connection.ModuleBInstanceId.ToString()));
			continue;
		}
		if (Connection.ModuleAInstanceId == Connection.ModuleBInstanceId)
		{
			AddError(Result.Errors, FString::Printf(TEXT("Недопустимая самосвязь модуля '%s'."), *Connection.ModuleAInstanceId.ToString()));
			continue;
		}

		const FShipModuleContactPoint* SocketA = ShipBuildDomainPrivate::FindSocketByName(*DefinitionA, Connection.ModuleASocketName);
		const FShipModuleContactPoint* SocketB = ShipBuildDomainPrivate::FindSocketByName(*DefinitionB, Connection.ModuleBSocketName);

		if (!SocketA)
		{
			AddError(Result.Errors, FString::Printf(
				TEXT("Socket '%s' не найден у instance '%s'."),
				*Connection.ModuleASocketName.ToString(),
				*Connection.ModuleAInstanceId.ToString()));
			continue;
		}
		if (!SocketB)
		{
			AddError(Result.Errors, FString::Printf(
				TEXT("Socket '%s' не найден у instance '%s'."),
				*Connection.ModuleBSocketName.ToString(),
				*Connection.ModuleBInstanceId.ToString()));
			continue;
		}

		if (!ShipBuildDomainPrivate::AreSocketTypesCompatible(SocketA->SocketType, SocketB->SocketType))
		{
			AddError(Result.Errors, FString::Printf(
				TEXT("Несовместимые типы сокетов: '%s' (%d) и '%s' (%d)."),
				*Connection.ModuleASocketName.ToString(),
				static_cast<int32>(SocketA->SocketType),
				*Connection.ModuleBSocketName.ToString(),
				static_cast<int32>(SocketB->SocketType)));
		}

		if (!AreModuleTypesCompatible(*DefinitionA, *DefinitionB)
			|| !AreModuleTypesCompatible(*DefinitionB, *DefinitionA))
		{
			AddError(Result.Errors, FString::Printf(
				TEXT("Несовместимые типы модулей в связи '%s' <-> '%s'."),
				*Connection.ModuleAInstanceId.ToString(),
				*Connection.ModuleBInstanceId.ToString()));
		}

		SocketUsageCount.FindOrAdd(
			FString::Printf(TEXT("%s::%s"), *Connection.ModuleAInstanceId.ToString(), *Connection.ModuleASocketName.ToString()))++;
		SocketUsageCount.FindOrAdd(
			FString::Printf(TEXT("%s::%s"), *Connection.ModuleBInstanceId.ToString(), *Connection.ModuleBSocketName.ToString()))++;
	}

	for (const TPair<FString, int32>& Entry : SocketUsageCount)
	{
		if (Entry.Value > 1)
		{
			AddError(Result.Errors, FString::Printf(TEXT("Сокет '%s' используется более одного раза."), *Entry.Key));
		}
	}

	Result.bIsValid = Result.Errors.Num() == 0;

	// Неблокирующие предупреждения по составу модулей (заглушки; баланс настраивается позже).
	if (DefinitionByInstance.Num() > 0)
	{
		bool bHasReactor = false;
		bool bHasBridge = false;
		bool bHasFuelTank = false;
		bool bHasOxygenTank = false;
		int32 EngineCount = 0;
		for (const TPair<FName, const UShipModuleDefinition*>& Pair : DefinitionByInstance)
		{
			if (!Pair.Value)
			{
				continue;
			}
			switch (Pair.Value->ModuleType)
			{
			case EShipModuleType::Reactor:
				bHasReactor = true;
				break;
			case EShipModuleType::Bridge:
				bHasBridge = true;
				break;
			case EShipModuleType::FuelTank:
				bHasFuelTank = true;
				break;
			case EShipModuleType::OxygenTank:
				bHasOxygenTank = true;
				break;
			case EShipModuleType::Engine:
				++EngineCount;
				break;
			default:
				break;
			}
		}
		if (!bHasReactor)
		{
			AddWarning(Result.Warnings, TEXT("Нет реактора: энергобаланс не задан (предупреждение)."));
		}
		if (!bHasBridge)
		{
			AddWarning(Result.Warnings, TEXT("Нет мостика: нет явного модуля управления (предупреждение)."));
		}
		if (!bHasFuelTank)
		{
			AddWarning(Result.Warnings, TEXT("Нет топливных баков (предупреждение)."));
		}
		if (!bHasOxygenTank)
		{
			AddWarning(Result.Warnings, TEXT("Нет кислородных баков (предупреждение)."));
		}
		if (Result.TotalMass > 500.0f && EngineCount == 0)
		{
			AddWarning(Result.Warnings, TEXT("Высокая масса при отсутствии двигателей: возможна низкая мобильность (предупреждение)."));
		}
	}

	return Result;
}

const FShipBuildModuleInstance* FShipBuildDomainModel::FindModuleInstance(const FName InstanceId) const
{
	return Modules.FindByPredicate([InstanceId](const FShipBuildModuleInstance& Module)
	{
		return Module.InstanceId == InstanceId;
	});
}

bool FShipBuildDomainModel::AreModuleTypesCompatible(
	const UShipModuleDefinition& Source,
	const UShipModuleDefinition& Target) const
{
	return ShipBuildDomainPrivate::IsTypeAllowedBySource(Source, Target.ModuleType);
}

void FShipBuildDomainModel::AddError(TArray<FString>& OutErrors, const FString& ErrorText) const
{
	OutErrors.Add(ErrorText);
}

void FShipBuildDomainModel::AddWarning(TArray<FString>& OutWarnings, const FString& WarningText) const
{
	OutWarnings.Add(WarningText);
}

// ----------------------------------------------------------------------------
// Debug / PIE smoke helper
// ----------------------------------------------------------------------------

static void RunShipBuildDebugScenario(const TArray<FString>& Args, UWorld* World)
{
	if (!World)
	{
		UE_LOG(LogShipBuildDomain, Warning, TEXT("ShipBuild.DebugScenario: World недоступен."));
		return;
	}

	const UGameInstance* GameInstance = World->GetGameInstance();
	if (!GameInstance)
	{
		UE_LOG(LogShipBuildDomain, Warning, TEXT("ShipBuild.DebugScenario: GameInstance недоступен."));
		return;
	}

	const UShipModuleCatalog* Catalog = GameInstance->GetSubsystem<UShipModuleCatalog>();
	if (!Catalog)
	{
		UE_LOG(LogShipBuildDomain, Warning, TEXT("ShipBuild.DebugScenario: UShipModuleCatalog недоступен."));
		return;
	}

	const bool bInvalidScenarioRequested = Args.Num() > 0 && Args[0].Equals(TEXT("invalid"), ESearchCase::IgnoreCase);

	const TArray<UShipModuleDefinition*> AllDefinitions = Catalog->GetAllModules();
	if (AllDefinitions.Num() == 0)
	{
		UE_LOG(LogShipBuildDomain, Warning, TEXT("ShipBuild.DebugScenario: каталог модулей пуст."));
		return;
	}

	const UShipModuleDefinition* FirstDef = nullptr;
	const UShipModuleDefinition* SecondDef = nullptr;
	const FShipModuleContactPoint* FirstSocket = nullptr;
	const FShipModuleContactPoint* SecondSocket = nullptr;

	if (!ShipBuildDomainPrivate::TryFindCompatiblePair(AllDefinitions, FirstDef, FirstSocket, SecondDef, SecondSocket))
	{
		UE_LOG(LogShipBuildDomain, Warning, TEXT("ShipBuild.DebugScenario: не найдена совместимая пара модулей в каталоге."));
		return;
	}

	FCatalogShipBuildModuleResolver Resolver(*Catalog);
	FShipBuildDomainModel BuildModel(Resolver);

	FString Error;
	if (!BuildModel.AddRootModule(TEXT("Root"), FirstDef->ModuleId, &Error))
	{
		UE_LOG(LogShipBuildDomain, Warning, TEXT("ShipBuild.DebugScenario: не удалось добавить root: %s"), *Error);
		return;
	}

	if (!BuildModel.AddAttachedModule(
		TEXT("Attached"),
		SecondDef->ModuleId,
		TEXT("Root"),
		SecondSocket->SocketName,
		FirstSocket->SocketName,
		&Error))
	{
		UE_LOG(LogShipBuildDomain, Warning, TEXT("ShipBuild.DebugScenario: не удалось добавить attached: %s"), *Error);
		return;
	}

	if (bInvalidScenarioRequested)
	{
		const UShipModuleDefinition* ThirdDefinition = ShipBuildDomainPrivate::FindAnyDefinitionWithSocket(AllDefinitions);
		if (!ThirdDefinition)
		{
			UE_LOG(LogShipBuildDomain, Warning, TEXT("ShipBuild.DebugScenario: не найден третий модуль для invalid-сценария."));
			return;
		}

		BuildModel.AddAttachedModule(
			TEXT("InvalidAttach"),
			ThirdDefinition->ModuleId,
			TEXT("Root"),
			ThirdDefinition->ContactPoints[0].SocketName,
			FirstSocket->SocketName,
			nullptr);
	}

	const FShipBuildValidationResult Validation = BuildModel.Validate();
	UE_LOG(LogShipBuildDomain, Log, TEXT("ShipBuild.DebugScenario[%s]: Valid=%s, Mass=%.2f, Modules=%d, Connections=%d"),
		bInvalidScenarioRequested ? TEXT("invalid") : TEXT("valid"),
		Validation.bIsValid ? TEXT("true") : TEXT("false"),
		Validation.TotalMass,
		BuildModel.GetModules().Num(),
		BuildModel.GetConnections().Num());

	for (const FString& ValidationError : Validation.Errors)
	{
		UE_LOG(LogShipBuildDomain, Warning, TEXT("  - %s"), *ValidationError);
	}
	for (const FString& ValidationWarning : Validation.Warnings)
	{
		UE_LOG(LogShipBuildDomain, Log, TEXT("  (предупреждение) %s"), *ValidationWarning);
	}
}

static FAutoConsoleCommandWithWorldAndArgs GShipBuildDebugScenarioCmd(
	TEXT("ShipBuild.DebugScenario"),
	TEXT("Проверяет домен сборки корабля. Аргумент: valid (по умолчанию) или invalid."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&RunShipBuildDebugScenario));
