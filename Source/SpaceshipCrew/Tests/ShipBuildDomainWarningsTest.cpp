// Автотест: предупреждения T02b не блокируют bIsValid при отсутствии ошибок стыковки.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "ShipBuildDomain.h"
#include "ShipModuleDefinition.h"

namespace ShipBuildDomainWarningsTestPrivate
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
	FShipBuildDomainWarningsTest,
	"SpaceshipCrew.ShipBuild.Warnings",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FShipBuildDomainWarningsTest::RunTest(const FString& Parameters)
{
	using namespace ShipBuildDomainWarningsTestPrivate;

	FTestResolver Resolver;

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

	Resolver.Add(ModuleA);
	Resolver.Add(ModuleB);

	FShipBuildDomainModel BuildModel(Resolver);
	FString Error;
	TestTrue(TEXT("AddRoot_A"), BuildModel.AddRootModule(TEXT("A"), ModuleA->ModuleId, &Error));
	TestTrue(TEXT("AddAttached_B"), BuildModel.AddAttachedModule(
		TEXT("B"), ModuleB->ModuleId, TEXT("A"), TEXT("Main"), TEXT("Main"), &Error));

	const FShipBuildValidationResult Result = BuildModel.Validate();
	TestTrue(TEXT("ValidGraph"), Result.bIsValid);
	TestEqual(TEXT("NoErrors"), Result.Errors.Num(), 0);
	TestTrue(TEXT("HasWarnings"), Result.Warnings.Num() > 0);

	return true;
}

#endif
