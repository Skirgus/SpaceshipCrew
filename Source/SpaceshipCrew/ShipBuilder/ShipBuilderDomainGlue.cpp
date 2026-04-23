#include "ShipBuilderDomainGlue.h"

#include "ShipBuildDomain.h"
#include "ShipModuleDefinition.h"
#include "ShipModuleTypes.h"

namespace ShipBuilderDomainGluePrivate
{
	static bool AreSocketTypesCompatible(const EShipModuleSocketType A, const EShipModuleSocketType B)
	{
		return A == EShipModuleSocketType::Universal
			|| B == EShipModuleSocketType::Universal
			|| A == B;
	}

	static bool IsTypeAllowedBySource(const UShipModuleDefinition& Source, const EShipModuleType TargetType)
	{
		// Пустой список трактуется как "без ограничений".
		return Source.CompatibleModuleTypes.Num() == 0
			|| Source.CompatibleModuleTypes.Contains(TargetType);
	}

	static bool AreDefinitionsCompatible(const UShipModuleDefinition& A, const UShipModuleDefinition& B)
	{
		return IsTypeAllowedBySource(A, B.ModuleType) && IsTypeAllowedBySource(B, A.ModuleType);
	}

	static bool TryFindCompatibleSocketPair(
		const UShipModuleDefinition& ExistingDef,
		const UShipModuleDefinition& NewDef,
		const TSet<FName>& UsedExistingSockets,
		FName& OutExistingSocket,
		FName& OutNewSocket)
	{
		if (!AreDefinitionsCompatible(ExistingDef, NewDef))
		{
			return false;
		}

		for (const FShipModuleContactPoint& ExistingCP : ExistingDef.GetResolvedContactPoints())
		{
			if (ExistingCP.SocketName.IsNone() || UsedExistingSockets.Contains(ExistingCP.SocketName))
			{
				continue;
			}

			for (const FShipModuleContactPoint& NewCP : NewDef.GetResolvedContactPoints())
			{
				if (NewCP.SocketName.IsNone())
				{
					continue;
				}

				if (AreSocketTypesCompatible(ExistingCP.SocketType, NewCP.SocketType))
				{
					OutExistingSocket = ExistingCP.SocketName;
					OutNewSocket = NewCP.SocketName;
					return true;
				}
			}
		}

		return false;
	}
}

bool SpaceshipCrew_BuildDomainFromDraftChain(
	const FShipBuilderDraftConfig& Draft,
	const IShipBuildModuleResolver& Resolver,
	FShipBuildDomainModel& OutModel,
	FString& OutError)
{
	if (Draft.ModuleIds.Num() == 0)
	{
		return true;
	}

	const FName FirstId = Draft.ModuleIds[0];
	const UShipModuleDefinition* FirstDef = Resolver.ResolveModule(FirstId);
	if (!FirstDef)
	{
		OutError = FString::Printf(TEXT("Модуль '%s' не найден в каталоге."), *FirstId.ToString());
		return false;
	}

	if (!OutModel.AddRootModule(TEXT("Draft0"), FirstId, &OutError))
	{
		return false;
	}

	TMap<FName, TSet<FName>> UsedSocketsByInstance;

	for (int32 Index = 1; Index < Draft.ModuleIds.Num(); ++Index)
	{
		const FName NewModuleId = Draft.ModuleIds[Index];
		const UShipModuleDefinition* NewDef = Resolver.ResolveModule(NewModuleId);
		if (!NewDef)
		{
			OutError = FString::Printf(TEXT("Модуль '%s' не найден в каталоге."), *NewModuleId.ToString());
			return false;
		}

		const FName PrevInstance = *FString::Printf(TEXT("Draft%d"), Index - 1);
		const FName NewInstance = *FString::Printf(TEXT("Draft%d"), Index);
		const UShipModuleDefinition* PrevDef = Resolver.ResolveModule(Draft.ModuleIds[Index - 1]);
		if (!PrevDef)
		{
			OutError = FString::Printf(TEXT("Модуль '%s' не найден в каталоге."), *Draft.ModuleIds[Index - 1].ToString());
			return false;
		}

		FName PrevSocket = NAME_None;
		FName NewSocket = NAME_None;
		const TSet<FName>& UsedPrevSockets = UsedSocketsByInstance.FindOrAdd(PrevInstance);
		if (!ShipBuilderDomainGluePrivate::TryFindCompatibleSocketPair(*PrevDef, *NewDef, UsedPrevSockets, PrevSocket, NewSocket))
		{
			OutError = FString::Printf(
				TEXT("Не найден свободный совместимый сокет для стыковки '%s' -> '%s'. Проверьте контактные точки и CompatibleModuleTypes."),
				*Draft.ModuleIds[Index - 1].ToString(),
				*NewModuleId.ToString());
			return false;
		}

		if (!OutModel.AddAttachedModule(NewInstance, NewModuleId, PrevInstance, NewSocket, PrevSocket, &OutError))
		{
			return false;
		}

		UsedSocketsByInstance.FindOrAdd(PrevInstance).Add(PrevSocket);
		UsedSocketsByInstance.FindOrAdd(NewInstance).Add(NewSocket);
	}

	return true;
}
