#include "SpaceshipCrewMenuRoute.h"

// Реализация текстов через NSLOCTEXT (пространство SpaceshipCrew) для единообразия локализации.

namespace SpaceshipCrewMenu
{
	TArray<ESpaceshipMenuRoute> GetOrderedMenuRoutes()
	{
		return {
			ESpaceshipMenuRoute::Constructor,
			ESpaceshipMenuRoute::Trainings,
			ESpaceshipMenuRoute::NewGame,
			ESpaceshipMenuRoute::Settings,
			ESpaceshipMenuRoute::Exit
		};
	}

	FText GetDisplayName(ESpaceshipMenuRoute Route)
	{
		switch (Route)
		{
		case ESpaceshipMenuRoute::Constructor:
			return NSLOCTEXT("SpaceshipCrew", "Menu_Constructor", "Конструктор");
		case ESpaceshipMenuRoute::Trainings:
			return NSLOCTEXT("SpaceshipCrew", "Menu_Trainings", "Тренировки");
		case ESpaceshipMenuRoute::NewGame:
			return NSLOCTEXT("SpaceshipCrew", "Menu_NewGame", "Новая игра");
		case ESpaceshipMenuRoute::Settings:
			return NSLOCTEXT("SpaceshipCrew", "Menu_Settings", "Настройки");
		case ESpaceshipMenuRoute::Exit:
			return NSLOCTEXT("SpaceshipCrew", "Menu_Exit", "Выход");
		default:
			return FText::GetEmpty();
		}
	}

	bool IsExitRoute(ESpaceshipMenuRoute Route)
	{
		return Route == ESpaceshipMenuRoute::Exit;
	}
}
