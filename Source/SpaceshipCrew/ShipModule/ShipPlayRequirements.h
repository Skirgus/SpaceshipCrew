#pragma once

#include "CoreMinimal.h"
#include "ShipModule/ShipModuleTypes.h"

class UShipModuleDefinition;

/**
 * Обязательные модули для старта игры (новая кампания).
 * Сохранение чертежа в конструкторе не требует этих типов.
 */
struct SPACESHIPCREW_API FShipPlayRequirements
{
	static constexpr int32 MandatoryTypeCount = 6;

	static const EShipModuleType* GetMandatoryTypes();
	static int32 GetMandatoryTypeCount() { return MandatoryTypeCount; }

	/** true, если в конфигурации есть хотя бы один модуль каждого обязательного типа. */
	static bool HasAllMandatoryModules(
		const TMap<FName, const UShipModuleDefinition*>& DefinitionsByInstance);

	/** Добавляет сообщения только об отсутствующих обязательных типах. */
	static void AppendMissingMandatoryModuleMessages(
		const TMap<FName, const UShipModuleDefinition*>& DefinitionsByInstance,
		TArray<FString>& OutMessages);
};
