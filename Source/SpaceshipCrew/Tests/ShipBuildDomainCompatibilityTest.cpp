// Автотест валидации совместимости в домене сборки корабля (T02b):
// проверяем валидную и невалидную стыковку модулей по типам сокетов/модулей.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "ShipBuildDomain.h"
#include "ShipModuleDefinition.h"

namespace ShipBuildDomainCompatibilityTestPrivate
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

	static UShipModuleDefinition* MakeDefinition(
		const FName ModuleId,
		const EShipModuleType ModuleType,
		const EShipModuleSocketType SocketType,
		const TArray<EShipModuleType>& CompatibleTypes)
	{
		UShipModuleDefinition* Definition = NewObject<UShipModuleDefinition>();
		Definition->ModuleId = ModuleId;
		Definition->ModuleType = ModuleType;
		Definition->DisplayName = FText::FromString(ModuleId.ToString());
		Definition->Mass = 100.0f;
		Definition->Size = FVector(300.0, 300.0, 300.0);
		Definition->CompatibleModuleTypes = CompatibleTypes;

		FShipModuleContactPoint ContactPoint;
		ContactPoint.SocketName = TEXT("Main");
		ContactPoint.SocketType = SocketType;
		Definition->ContactPoints.Add(ContactPoint);
		return Definition;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShipBuildDomainCompatibilityTest,
	"SpaceshipCrew.ShipBuild.Compatibility",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FShipBuildDomainCompatibilityTest::RunTest(const FString& Parameters)
{
	using namespace ShipBuildDomainCompatibilityTestPrivate;

	FTestResolver Resolver;

	// A и B совместимы по типам модулей и горизонтальным сокетам.
	UShipModuleDefinition* ModuleA = MakeDefinition(
		TEXT("BridgeA"),
		EShipModuleType::Bridge,
		EShipModuleSocketType::Horizontal,
		{ EShipModuleType::Corridor });
	UShipModuleDefinition* ModuleB = MakeDefinition(
		TEXT("CorridorB"),
		EShipModuleType::Corridor,
		EShipModuleSocketType::Horizontal,
		{ EShipModuleType::Bridge, EShipModuleType::Corridor });

	// C заведомо несовместим: вертикальный сокет и запрещенный тип цели.
	UShipModuleDefinition* ModuleC = MakeDefinition(
		TEXT("ScienceC"),
		EShipModuleType::ScienceLab,
		EShipModuleSocketType::Vertical,
		{ EShipModuleType::ScienceLab });

	Resolver.Add(ModuleA);
	Resolver.Add(ModuleB);
	Resolver.Add(ModuleC);

	FShipBuildDomainModel BuildModel(Resolver);

	FString Error;
	TestTrue(TEXT("AddRoot_A"), BuildModel.AddRootModule(TEXT("A"), ModuleA->ModuleId, &Error));
	TestTrue(TEXT("AddAttached_B"), BuildModel.AddAttachedModule(
		TEXT("B"), ModuleB->ModuleId, TEXT("A"), TEXT("Main"), TEXT("Main"), &Error));

	const FShipBuildValidationResult ValidResult = BuildModel.Validate();
	TestTrue(TEXT("AB_ShouldBeValid"), ValidResult.bIsValid);
	TestEqual(TEXT("AB_NoErrors"), ValidResult.Errors.Num(), 0);

	// Заменяем B на C с сохранением существующей связи -> конфигурация должна стать невалидной.
	TestTrue(TEXT("Replace_B_To_C"), BuildModel.ReplaceModule(TEXT("B"), ModuleC->ModuleId, &Error));
	const FShipBuildValidationResult InvalidResult = BuildModel.Validate();
	TestFalse(TEXT("AC_ShouldBeInvalid"), InvalidResult.bIsValid);
	TestTrue(TEXT("AC_ShouldHaveErrors"), InvalidResult.Errors.Num() > 0);

	return true;
}

#endif
