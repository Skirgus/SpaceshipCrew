#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "SpaceshipCrewMenuRoute.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpaceshipCrewMenuRoutingRegistryTest,
	"SpaceshipCrew.Menu.RoutingRegistry",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FSpaceshipCrewMenuRoutingRegistryTest::RunTest(const FString& Parameters)
{
	using namespace SpaceshipCrewMenu;

	const TArray<ESpaceshipMenuRoute> Routes = GetOrderedMenuRoutes();
	TestEqual(TEXT("MainMenuButtonCount"), Routes.Num(), 5);

	for (const ESpaceshipMenuRoute Route : Routes)
	{
		const FText Label = GetDisplayName(Route);
		TestTrue(*FString::Printf(TEXT("DisplayNameNotEmpty_%d"), static_cast<int32>(Route)), !Label.IsEmpty());
	}

	TestTrue(TEXT("ExitFlag"), IsExitRoute(ESpaceshipMenuRoute::Exit));
	TestFalse(TEXT("ConstructorNotExit"), IsExitRoute(ESpaceshipMenuRoute::Constructor));

	return true;
}

#endif
