// Автотест детерминированного расчета массы в домене сборки корабля (T02b).

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "ShipBuildDomain.h"
#include "ShipModuleDefinition.h"

namespace ShipBuildDomainMassTestPrivate
{
	class FTestResolver final : public IShipBuildModuleResolver
	{
	public:
		void Add(UShipModuleDefinition* Definition)
		{
			if (Definition)
			{
				DefinitionsById.Add(Definition->ModuleId, Definition);
			}
		}

		virtual const UShipModuleDefinition* ResolveModule(const FName ModuleId) const override
		{
			if (const TObjectPtr<UShipModuleDefinition>* Found = DefinitionsById.Find(ModuleId))
			{
				return Found->Get();
			}
			return nullptr;
		}

	private:
		TMap<FName, TObjectPtr<UShipModuleDefinition>> DefinitionsById;
	};

	static UShipModuleDefinition* MakeDefinition(const FName ModuleId, const float Mass)
	{
		UShipModuleDefinition* Definition = NewObject<UShipModuleDefinition>();
		Definition->ModuleId = ModuleId;
		Definition->ModuleType = EShipModuleType::Corridor;
		Definition->DisplayName = FText::FromString(ModuleId.ToString());
		Definition->Mass = Mass;
		Definition->Size = FVector(300.0, 300.0, 300.0);
		Definition->CompatibleModuleTypes = { EShipModuleType::Corridor };

		FShipModuleContactPoint ContactPoint;
		ContactPoint.SocketName = TEXT("Main");
		ContactPoint.SocketType = EShipModuleSocketType::Horizontal;
		Definition->ContactPoints.Add(ContactPoint);
		return Definition;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShipBuildDomainMassTest,
	"SpaceshipCrew.ShipBuild.Mass",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FShipBuildDomainMassTest::RunTest(const FString& Parameters)
{
	using namespace ShipBuildDomainMassTestPrivate;

	FTestResolver Resolver;
	UShipModuleDefinition* ModuleA = MakeDefinition(TEXT("MassA"), 10.5f);
	UShipModuleDefinition* ModuleB = MakeDefinition(TEXT("MassB"), 20.25f);
	UShipModuleDefinition* ModuleC = MakeDefinition(TEXT("MassC"), 30.0f);
	Resolver.Add(ModuleA);
	Resolver.Add(ModuleB);
	Resolver.Add(ModuleC);

	// Первая конфигурация.
	FShipBuildDomainModel BuildModel1(Resolver);
	FString Error;
	TestTrue(TEXT("M1_AddRootB"), BuildModel1.AddRootModule(TEXT("B"), ModuleB->ModuleId, &Error));
	TestTrue(TEXT("M1_AddA"), BuildModel1.AddAttachedModule(TEXT("A"), ModuleA->ModuleId, TEXT("B"), TEXT("Main"), TEXT("Main"), &Error));
	TestTrue(TEXT("M1_AddC"), BuildModel1.AddAttachedModule(TEXT("C"), ModuleC->ModuleId, TEXT("A"), TEXT("Main"), TEXT("Main"), &Error));

	// Вторая конфигурация с другим порядком мутаций.
	FShipBuildDomainModel BuildModel2(Resolver);
	TestTrue(TEXT("M2_AddRootA"), BuildModel2.AddRootModule(TEXT("A"), ModuleA->ModuleId, &Error));
	TestTrue(TEXT("M2_AddC"), BuildModel2.AddAttachedModule(TEXT("C"), ModuleC->ModuleId, TEXT("A"), TEXT("Main"), TEXT("Main"), &Error));
	TestTrue(TEXT("M2_AddB"), BuildModel2.AddAttachedModule(TEXT("B"), ModuleB->ModuleId, TEXT("C"), TEXT("Main"), TEXT("Main"), &Error));

	const float ExpectedMass = 60.75f;
	const float TotalMass1 = BuildModel1.GetTotalMass();
	const float TotalMass2 = BuildModel2.GetTotalMass();

	TestTrue(TEXT("Mass1_EqualsExpected"), FMath::IsNearlyEqual(TotalMass1, ExpectedMass, KINDA_SMALL_NUMBER));
	TestTrue(TEXT("Mass2_EqualsExpected"), FMath::IsNearlyEqual(TotalMass2, ExpectedMass, KINDA_SMALL_NUMBER));
	TestTrue(TEXT("Mass_DeterministicAcrossMutationOrder"), FMath::IsNearlyEqual(TotalMass1, TotalMass2, KINDA_SMALL_NUMBER));

	return true;
}

#endif
