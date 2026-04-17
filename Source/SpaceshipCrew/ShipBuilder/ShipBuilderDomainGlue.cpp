#include "ShipBuilderDomainGlue.h"

#include "ShipBuildDomain.h"
#include "ShipModuleDefinition.h"

namespace ShipBuilderDomainGluePrivate
{
	static FName GetFirstSocketName(const UShipModuleDefinition& Definition)
	{
		if (Definition.ContactPoints.Num() > 0)
		{
			return Definition.ContactPoints[0].SocketName;
		}
		return NAME_None;
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

		const FName NewSocket = ShipBuilderDomainGluePrivate::GetFirstSocketName(*NewDef);
		const UShipModuleDefinition* PrevDef = Resolver.ResolveModule(Draft.ModuleIds[Index - 1]);
		const FName PrevSocket = PrevDef ? ShipBuilderDomainGluePrivate::GetFirstSocketName(*PrevDef) : NAME_None;

		if (NewSocket.IsNone() || PrevSocket.IsNone())
		{
			OutError = TEXT("У модуля нет контактных точек для автоматической цепочки.");
			return false;
		}

		if (!OutModel.AddAttachedModule(NewInstance, NewModuleId, PrevInstance, NewSocket, PrevSocket, &OutError))
		{
			return false;
		}
	}

	return true;
}
