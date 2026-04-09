#pragma once

#include "CoreMinimal.h"
#include "SpaceshipCrewMenuRoute.generated.h"

/** Идентификаторы пунктов главного меню (порядок кнопок задаётся в GetOrderedMenuRoutes). */
UENUM()
enum class ESpaceshipMenuRoute : uint8
{
	/** Экран конструктора корабля (пока заглушка). */
	Constructor,
	/** Режим тренировок (пока заглушка). */
	Trainings,
	/** Новая кампания (пока заглушка). */
	NewGame,
	/** Настройки (пока заглушка). */
	Settings,
	/** Выход из игры. */
	Exit
};

/** Статические подписи и перечисление маршрутов для UI и автотестов. */
namespace SpaceshipCrewMenu
{
	TArray<ESpaceshipMenuRoute> GetOrderedMenuRoutes();
	FText GetDisplayName(ESpaceshipMenuRoute Route);
	bool IsExitRoute(ESpaceshipMenuRoute Route);
}
