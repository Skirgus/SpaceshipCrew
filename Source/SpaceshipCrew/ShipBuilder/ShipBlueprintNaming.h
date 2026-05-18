#pragma once

#include "CoreMinimal.h"

/**
 * Идентификаторы сохранений игрока: транслитерация кириллицы + стабильный суффикс по полному имени.
 */
namespace ShipBlueprintNaming
{
	/** Читаемая часть id (латиница, цифры, подчёркивания) без суффикса хеша. */
	SPACESHIPCREW_API FString MakeReadableSlug(const FString& DisplayName);

	/** Стабильный 8-символьный суффикс по полному DisplayName (различает одноимённые транслиты). */
	SPACESHIPCREW_API FString MakeStableHashSuffix(const FString& DisplayName);

	/** ShipId для сохранения: {slug}_{hash}, slug не короче «ship» при пустом вводе. */
	SPACESHIPCREW_API FName MakeShipIdFromDisplayName(const FString& DisplayName);

	/** Вариант с числовым суффиксом при коллизии файлов (_1, _2, …). */
	SPACESHIPCREW_API FName MakeUniquePlayerShipId(const FString& BaseDisplayName, int32 NumericSuffix);

	/**
	 * Проверяет ShipId для имени файла: только [A-Za-z0-9_], без .. и разделителей пути.
	 * @return false — id небезопасен для записи на диск.
	 */
	SPACESHIPCREW_API bool TrySanitizeShipIdForFilename(FName ShipId, FString& OutFileBaseName);
}
