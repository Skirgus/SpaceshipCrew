// Автотест цепочки ShipBuilderDomainGlue:
// проверяем, что при автосборке используется свободный совместимый сокет,
// а не всегда первый сокет модуля.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "ShipBuilder/ShipBuilderDomainGlue.h"
#include "ShipModule/ShipBuildDomain.h"
#include "ShipModule/ShipModuleDefinition.h"
#include "ShipModule/ShipModuleVisualOverride.h"

namespace ShipBuilderDomainGlueChainSocketsTestPrivate
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
		const TArray<FName>& SocketNames,
		const TArray<EShipModuleType>& CompatibleTypes)
	{
		UShipModuleDefinition* Definition = NewObject<UShipModuleDefinition>();
		Definition->ModuleId = ModuleId;
		Definition->ModuleType = ModuleType;
		Definition->DisplayName = FText::FromString(ModuleId.ToString());
		Definition->Mass = 100.0f;
		Definition->Size = FVector(400.0, 400.0, 300.0);
		Definition->CompatibleModuleTypes = CompatibleTypes;

		for (int32 Index = 0; Index < SocketNames.Num(); ++Index)
		{
			FShipModuleContactPoint CP;
			CP.SocketName = SocketNames[Index];
			CP.SocketType = EShipModuleSocketType::Horizontal;
			CP.RelativeLocation = FVector(Index * 10.0f, 0.0f, 0.0f);
			Definition->ContactPoints.Add(CP);
		}

		return Definition;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShipBuilderDomainGlueChainSocketsTest,
	"SpaceshipCrew.ShipBuilder.DomainGlue.UsesFreeSocketInChain",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FShipBuilderDomainGlueChainSocketsTest::RunTest(const FString& Parameters)
{
	using namespace ShipBuilderDomainGlueChainSocketsTestPrivate;

	FTestResolver Resolver;

	// Root: один сокет.
	UShipModuleDefinition* Root = MakeDefinition(
		TEXT("RootBridge"),
		EShipModuleType::Bridge,
		{ TEXT("Back") },
		{ EShipModuleType::Corridor });

	// Middle: два сокета; первый будет занят первой связью, второй нужен для следующего модуля.
	UShipModuleDefinition* Middle = MakeDefinition(
		TEXT("MidCorridor"),
		EShipModuleType::Corridor,
		{ TEXT("A"), TEXT("B") },
		{ EShipModuleType::Bridge, EShipModuleType::Corridor, EShipModuleType::Airlock });

	// Tail: односторонний "уличный" модуль (шлюз) с одним сокетом.
	UShipModuleDefinition* Tail = MakeDefinition(
		TEXT("AirlockTail"),
		EShipModuleType::Airlock,
		{ TEXT("In") },
		{ EShipModuleType::Corridor });

	Resolver.Add(Root);
	Resolver.Add(Middle);
	Resolver.Add(Tail);

	FShipBuilderDraftConfig Draft;
	Draft.ModuleIds = { Root->ModuleId, Middle->ModuleId, Tail->ModuleId };

	FShipBuildDomainModel Model(Resolver);
	FString Error;
	const bool bBuilt = SpaceshipCrew_BuildDomainFromDraftChain(Draft, Resolver, Model, Error);
	TestTrue(FString::Printf(TEXT("BuildChainSuccess: %s"), *Error), bBuilt);
	if (!bBuilt)
	{
		return false;
	}

	const FShipBuildValidationResult Validation = Model.Validate();
	TestTrue(TEXT("ChainShouldBeValid"), Validation.bIsValid);
	TestEqual(TEXT("ExpectedTwoConnections"), Model.GetConnections().Num(), 2);

	// У среднего модуля должны использоваться разные сокеты в двух связях.
	TSet<FName> MidSockets;
	for (const FShipBuildModuleConnection& Connection : Model.GetConnections())
	{
		if (Connection.ModuleAInstanceId == TEXT("Draft1"))
		{
			MidSockets.Add(Connection.ModuleASocketName);
		}
		if (Connection.ModuleBInstanceId == TEXT("Draft1"))
		{
			MidSockets.Add(Connection.ModuleBSocketName);
		}
	}
	TestEqual(TEXT("MiddleUsesTwoDistinctSockets"), MidSockets.Num(), 2);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShipBuilderDomainGlueChainResolvesOverrideSocketsTest,
	"SpaceshipCrew.ShipBuilder.DomainGlue.ResolvesOverrideSockets",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FShipBuilderDomainGlueChainResolvesOverrideSocketsTest::RunTest(const FString& Parameters)
{
	using namespace ShipBuilderDomainGlueChainSocketsTestPrivate;

	FTestResolver Resolver;

	UShipModuleDefinition* Root = MakeDefinition(
		TEXT("RootBridgeOverride"),
		EShipModuleType::Bridge,
		{ TEXT("Front") },
		{ EShipModuleType::Corridor });

	UShipModuleDefinition* Middle = MakeDefinition(
		TEXT("MidCorridorOverride"),
		EShipModuleType::Corridor,
		{ TEXT("A"), TEXT("B") },
		{ EShipModuleType::Bridge, EShipModuleType::Corridor });

	UShipModuleVisualOverride* MiddleOverride = NewObject<UShipModuleVisualOverride>();
	MiddleOverride->bOverrideContactPoints = true;
	MiddleOverride->ContactPointsOverride.Reset();
	FShipModuleContactPoint OverrideCp;
	OverrideCp.SocketName = TEXT("OnlyOverride");
	OverrideCp.SocketType = EShipModuleSocketType::Horizontal;
	MiddleOverride->ContactPointsOverride.Add(OverrideCp);
	Middle->VisualOverride = MiddleOverride;

	UShipModuleDefinition* Tail = MakeDefinition(
		TEXT("TailCorridorOverride"),
		EShipModuleType::Corridor,
		{ TEXT("Rear") },
		{ EShipModuleType::Corridor });

	Resolver.Add(Root);
	Resolver.Add(Middle);
	Resolver.Add(Tail);

	FShipBuilderDraftConfig Draft;
	Draft.ModuleIds = { Root->ModuleId, Middle->ModuleId, Tail->ModuleId };

	FShipBuildDomainModel Model(Resolver);
	FString Error;
	const bool bBuilt = SpaceshipCrew_BuildDomainFromDraftChain(Draft, Resolver, Model, Error);
	TestTrue(FString::Printf(TEXT("BuildChainWithOverrideSockets: %s"), *Error), bBuilt);
	if (!bBuilt)
	{
		return false;
	}

	TSet<FName> MiddleUsedSockets;
	for (const FShipBuildModuleConnection& Connection : Model.GetConnections())
	{
		if (Connection.ModuleAInstanceId == TEXT("Draft1"))
		{
			MiddleUsedSockets.Add(Connection.ModuleASocketName);
		}
		if (Connection.ModuleBInstanceId == TEXT("Draft1"))
		{
			MiddleUsedSockets.Add(Connection.ModuleBSocketName);
		}
	}

	TestEqual(TEXT("MiddleShouldUseSingleOverrideSocket"), MiddleUsedSockets.Num(), 1);
	TestTrue(TEXT("MiddleUsesOverrideSocketName"), MiddleUsedSockets.Contains(TEXT("OnlyOverride")));
	return true;
}

#endif

