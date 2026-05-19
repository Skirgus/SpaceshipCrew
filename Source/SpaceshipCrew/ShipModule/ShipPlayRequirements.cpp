#include "ShipModule/ShipPlayRequirements.h"

#include "ShipModule/ShipModuleDefinition.h"

namespace ShipPlayRequirementsPrivate
{
	static constexpr EShipModuleType MandatoryTypes[] = {
		EShipModuleType::Reactor,
		EShipModuleType::Engine,
		EShipModuleType::FuelTank,
		EShipModuleType::OxygenTank,
		EShipModuleType::Airlock,
		EShipModuleType::Bridge,
	};

	static bool HasModuleType(
		const TMap<FName, const UShipModuleDefinition*>& DefinitionsByInstance,
		const EShipModuleType Type)
	{
		for (const TPair<FName, const UShipModuleDefinition*>& Pair : DefinitionsByInstance)
		{
			if (Pair.Value && Pair.Value->ModuleType == Type)
			{
				return true;
			}
		}
		return false;
	}

	static FString MessageForMissingType(const EShipModuleType Type)
	{
		switch (Type)
		{
		case EShipModuleType::Reactor:
			return TEXT("Нет реактора: корабль не готов к полёту.");
		case EShipModuleType::Engine:
			return TEXT("Нет двигателя: корабль не готов к полёту.");
		case EShipModuleType::FuelTank:
			return TEXT("Нет топливного бака: корабль не готов к полёту.");
		case EShipModuleType::OxygenTank:
			return TEXT("Нет кислородного бака: корабль не готов к полёту.");
		case EShipModuleType::Airlock:
			return TEXT("Нет воздушного шлюза: корабль не готов к полёту.");
		case EShipModuleType::Bridge:
			return TEXT("Нет командирского мостика: корабль не готов к полёту.");
		default:
			return TEXT("Отсутствует обязательный модуль.");
		}
	}
}

const EShipModuleType* FShipPlayRequirements::GetMandatoryTypes()
{
	return ShipPlayRequirementsPrivate::MandatoryTypes;
}

bool FShipPlayRequirements::HasAllMandatoryModules(
	const TMap<FName, const UShipModuleDefinition*>& DefinitionsByInstance)
{
	for (const EShipModuleType Type : ShipPlayRequirementsPrivate::MandatoryTypes)
	{
		if (!ShipPlayRequirementsPrivate::HasModuleType(DefinitionsByInstance, Type))
		{
			return false;
		}
	}
	return true;
}

void FShipPlayRequirements::AppendMissingMandatoryModuleMessages(
	const TMap<FName, const UShipModuleDefinition*>& DefinitionsByInstance,
	TArray<FString>& OutMessages)
{
	for (const EShipModuleType Type : ShipPlayRequirementsPrivate::MandatoryTypes)
	{
		if (!ShipPlayRequirementsPrivate::HasModuleType(DefinitionsByInstance, Type))
		{
			OutMessages.Add(ShipPlayRequirementsPrivate::MessageForMissingType(Type));
		}
	}
}
