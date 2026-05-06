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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShipBuildDomainDefaultSocketsTest,
	"SpaceshipCrew.ShipBuild.DefaultSocketsFallback",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FShipBuildDomainDefaultSocketsTest::RunTest(const FString& Parameters)
{
	using namespace ShipBuildDomainCompatibilityTestPrivate;

	FTestResolver Resolver;
	UShipModuleDefinition* A = NewObject<UShipModuleDefinition>();
	A->ModuleId = TEXT("DefaultA");
	A->ModuleType = EShipModuleType::Bridge;
	A->DisplayName = FText::FromString(TEXT("DefaultA"));
	A->Mass = 100.0f;
	A->Size = FVector(300.0, 300.0, 300.0);
	A->CompatibleModuleTypes = { EShipModuleType::Corridor };
	A->ContactPoints.Reset();

	UShipModuleDefinition* B = NewObject<UShipModuleDefinition>();
	B->ModuleId = TEXT("DefaultB");
	B->ModuleType = EShipModuleType::Corridor;
	B->DisplayName = FText::FromString(TEXT("DefaultB"));
	B->Mass = 100.0f;
	B->Size = FVector(300.0, 300.0, 300.0);
	B->CompatibleModuleTypes = { EShipModuleType::Bridge };
	B->ContactPoints.Reset();

	Resolver.Add(A);
	Resolver.Add(B);

	FShipBuildDomainModel BuildModel(Resolver);
	FString Error;
	TestTrue(TEXT("AddRoot_DefaultA"), BuildModel.AddRootModule(TEXT("A"), A->ModuleId, &Error));
	TestTrue(TEXT("AddRoot_DefaultB"), BuildModel.AddRootModule(TEXT("B"), B->ModuleId, &Error));
	TestTrue(TEXT("AddConnection_DefaultFrontBack"), BuildModel.AddConnectionBetweenExisting(TEXT("A"), TEXT("Front"), TEXT("B"), TEXT("Back"), &Error));

	const FShipBuildValidationResult Validation = BuildModel.Validate();
	TestTrue(TEXT("DefaultSocketsFallback_Valid"), Validation.bIsValid);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShipBuildDomainVerticalDirectionTest,
	"SpaceshipCrew.ShipBuild.VerticalDirection",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FShipBuildDomainVerticalDirectionTest::RunTest(const FString& Parameters)
{
	using namespace ShipBuildDomainCompatibilityTestPrivate;

	FTestResolver Resolver;
	UShipModuleDefinition* Lower = MakeDefinition(
		TEXT("Lower"),
		EShipModuleType::Corridor,
		EShipModuleSocketType::Vertical,
		{ EShipModuleType::Corridor });
	Lower->ContactPoints[0].SocketName = TEXT("Top");
	Lower->ContactPoints[0].RelativeLocation = FVector(0.0f, 0.0f, 150.0f);

	UShipModuleDefinition* Upper = MakeDefinition(
		TEXT("Upper"),
		EShipModuleType::Corridor,
		EShipModuleSocketType::Vertical,
		{ EShipModuleType::Corridor });
	Upper->ContactPoints[0].SocketName = TEXT("Bottom");
	Upper->ContactPoints[0].RelativeLocation = FVector(0.0f, 0.0f, -150.0f);

	Resolver.Add(Lower);
	Resolver.Add(Upper);

	FShipBuildDomainModel BuildModel(Resolver);
	FString Error;
	TestTrue(TEXT("AddRoot_Lower"), BuildModel.AddRootModule(TEXT("Lower"), Lower->ModuleId, &Error));
	TestTrue(TEXT("AddRoot_Upper"), BuildModel.AddRootModule(TEXT("Upper"), Upper->ModuleId, &Error));
	TestTrue(TEXT("AddConnection_ValidTopBottom"), BuildModel.AddConnectionBetweenExisting(TEXT("Lower"), TEXT("Top"), TEXT("Upper"), TEXT("Bottom"), &Error));
	TestTrue(TEXT("TopBottom_Valid"), BuildModel.Validate().bIsValid);

	FShipBuildDomainModel InvalidModel(Resolver);
	TestTrue(TEXT("AddRoot_Lower_Invalid"), InvalidModel.AddRootModule(TEXT("Lower"), Lower->ModuleId, &Error));
	TestTrue(TEXT("AddRoot_Upper_Invalid"), InvalidModel.AddRootModule(TEXT("Upper"), Upper->ModuleId, &Error));
	TestTrue(TEXT("AddConnection_InvalidTopTop"), InvalidModel.AddConnectionBetweenExisting(TEXT("Lower"), TEXT("Top"), TEXT("Upper"), TEXT("Top"), &Error));
	TestFalse(TEXT("TopTop_Invalid"), InvalidModel.Validate().bIsValid);
	return true;
}

#endif
