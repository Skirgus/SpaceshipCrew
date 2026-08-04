// Автотест: готовность корабля к полёту (обязательные модули + целостность сборки).

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "ShipBuilder/ShipBuilderDomainGlue.h"
#include "ShipModule/ShipBuildDomain.h"
#include "ShipModule/ShipModuleDefinition.h"
#include "ShipModule/ShipPlayRequirements.h"

namespace ShipBuildDomainPlayReadinessTestPrivate
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

	static TArray<EShipModuleType> AllDockableTypes()
	{
		return {
			EShipModuleType::Bridge,
			EShipModuleType::Corridor,
			EShipModuleType::Reactor,
			EShipModuleType::Engine,
			EShipModuleType::FuelTank,
			EShipModuleType::OxygenTank,
			EShipModuleType::Airlock,
		};
	}

	static UShipModuleDefinition* MakeDefinition(const FName ModuleId, const EShipModuleType ModuleType)
	{
		UShipModuleDefinition* Definition = NewObject<UShipModuleDefinition>();
		Definition->ModuleId = ModuleId;
		Definition->ModuleType = ModuleType;
		Definition->DisplayName = FText::FromString(ModuleId.ToString());
		Definition->Mass = 100.0f;
		Definition->Size = FVector(400.0, 400.0, 300.0);
		Definition->CompatibleModuleTypes = AllDockableTypes();
		Definition->EnsureContactPointsPopulatedIfNoAuthoringOverride();
		return Definition;
	}

	static void RegisterMinimalPlaySet(FTestResolver& Resolver)
	{
		Resolver.Add(MakeDefinition(TEXT("PlayBridge"), EShipModuleType::Bridge));
		Resolver.Add(MakeDefinition(TEXT("PlayReactor"), EShipModuleType::Reactor));
		Resolver.Add(MakeDefinition(TEXT("PlayFuel"), EShipModuleType::FuelTank));
		Resolver.Add(MakeDefinition(TEXT("PlayOxygen"), EShipModuleType::OxygenTank));
		Resolver.Add(MakeDefinition(TEXT("PlayEngine"), EShipModuleType::Engine));
		Resolver.Add(MakeDefinition(TEXT("PlayAirlock"), EShipModuleType::Airlock));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShipBuildDomainPlayReadinessIncompleteTest,
	"SpaceshipCrew.ShipBuild.PlayReadiness.Incomplete",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FShipBuildDomainPlayReadinessIncompleteTest::RunTest(const FString& Parameters)
{
	using namespace ShipBuildDomainPlayReadinessTestPrivate;

	FTestResolver Resolver;
	Resolver.Add(MakeDefinition(TEXT("BridgeA"), EShipModuleType::Bridge));
	Resolver.Add(MakeDefinition(TEXT("CorridorB"), EShipModuleType::Corridor));

	FShipBuildDomainModel Model(Resolver);
	FString Error;
	TestTrue(TEXT("AddRoot"), Model.AddRootModule(TEXT("A"), FName(TEXT("BridgeA")), &Error));
	TestTrue(TEXT("AddCorridor"), Model.AddAttachedModule(
		TEXT("B"), FName(TEXT("CorridorB")), TEXT("A"), TEXT("Front"), TEXT("Front"), &Error));

	const FShipBuildValidationResult Result = Model.Validate();
	TestTrue(TEXT("ValidTopology"), Result.bIsValid);
	TestFalse(TEXT("NotPlayReady"), Result.bIsPlayReady);
	TestEqual(TEXT("NoWarnings"), Result.Warnings.Num(), 0);
	TestTrue(TEXT("HasPlayBlockers"), Result.PlayBlockers.Num() > Result.Errors.Num());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShipBuildDomainPlayReadinessCompleteTest,
	"SpaceshipCrew.ShipBuild.PlayReadiness.Complete",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FShipBuildDomainPlayReadinessCompleteTest::RunTest(const FString& Parameters)
{
	using namespace ShipBuildDomainPlayReadinessTestPrivate;

	FTestResolver Resolver;
	RegisterMinimalPlaySet(Resolver);

	FShipBuilderDraftConfig Draft;
	Draft.ModuleIds = {
		FName(TEXT("PlayBridge")),
		FName(TEXT("PlayReactor")),
		FName(TEXT("PlayFuel")),
		FName(TEXT("PlayOxygen")),
		FName(TEXT("PlayEngine")),
		FName(TEXT("PlayAirlock")),
	};

	FShipBuildDomainModel Model(Resolver);
	FString Error;
	TestTrue(TEXT("BuildChain"), SpaceshipCrew_BuildDomainFromDraftChain(Draft, Resolver, Model, Error));

	const FShipBuildValidationResult Result = Model.Validate();
	TestTrue(TEXT("ValidTopology"), Result.bIsValid);
	TestTrue(TEXT("PlayReady"), Result.bIsPlayReady);
	TestEqual(TEXT("NoPlayBlockers"), Result.PlayBlockers.Num(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShipPlayRequirementsMandatoryCountTest,
	"SpaceshipCrew.ShipBuild.PlayRequirements.MandatoryCount",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FShipPlayRequirementsMandatoryCountTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("MandatoryCount"), FShipPlayRequirements::GetMandatoryTypeCount(), 6);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
