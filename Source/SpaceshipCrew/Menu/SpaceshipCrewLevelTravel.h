#pragma once

#include "CoreMinimal.h"

/**
 * Пути уровней и query-параметры GameMode для OpenLevel (T02c-1).
 * Используется тот же ассет карты, что и в DefaultEngine.ini, смена режима через ?game=.
 */
namespace SpaceshipCrewLevelTravel
{
	/** Карта по умолчанию из проекта (см. GameDefaultMap). */
	inline const TCHAR* GetPlayMapPackagePath()
	{
		return TEXT("/Engine/Maps/Templates/Template_Default");
	}

	inline FString GetMainMenuGameOptions()
	{
		return FString::Printf(
			TEXT("?game=%s"),
			TEXT("/Script/SpaceshipCrew.SpaceshipCrewMenuGameMode"));
	}

	inline FString GetShipBuilderGameOptions()
	{
		return FString::Printf(
			TEXT("?game=%s"),
			TEXT("/Script/SpaceshipCrew.SpaceshipShipBuilderGameMode"));
	}

	/** Карта тренировки инженера (создаётся в Content; до готовности — Template_Default). */
	inline const TCHAR* GetEngineerTrainingMapPackagePath()
	{
		return TEXT("/Game/Maps/Training/EngineerTraining");
	}

	inline FString GetTrainingGameOptions()
	{
		return FString::Printf(
			TEXT("?game=%s"),
			TEXT("/Script/SpaceshipCrew.SpaceshipCrewTrainingGameMode"));
	}
}
