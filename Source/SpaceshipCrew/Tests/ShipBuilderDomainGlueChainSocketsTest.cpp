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
		const TArray<EShipModuleType>& CompatibleTypes)
	{
		UShipModuleDefinition* Definition = NewObject<UShipModuleDefinition>();
		Definition->ModuleId = ModuleId;
		Definition->ModuleType = ModuleType;
		Definition->DisplayName = FText::FromString(ModuleId.ToString());
		Definition->Mass = 100.0f;
		Definition->Size = FVector(400.0, 400.0, 300.0);
		Definition->CompatibleModuleTypes = CompatibleTypes;

		Definition->SyncCellSizeAndSizeFromLegacy();
		Definition->EnsureContactPointsPopulatedIfNoAuthoringOverride();
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
		{ EShipModuleType::Corridor });

	// Middle: два сокета; первый будет занят первой связью, второй нужен для следующего модуля.
	UShipModuleDefinition* Middle = MakeDefinition(
		TEXT("MidCorridor"),
		EShipModuleType::Corridor,
		{ EShipModuleType::Bridge, EShipModuleType::Corridor, EShipModuleType::Airlock });

	// Tail: односторонний "уличный" модуль (шлюз) с одним сокетом.
	UShipModuleDefinition* Tail = MakeDefinition(
		TEXT("AirlockTail"),
		EShipModuleType::Airlock,
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
		{ EShipModuleType::Corridor });

	UShipModuleDefinition* Middle = MakeDefinition(
		TEXT("MidCorridorOverride"),
		EShipModuleType::Corridor,
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShipBuilderDomainGluePlacedModulesTest,
	"SpaceshipCrew.ShipBuilder.DomainGlue.PlacedModules",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FShipBuilderDomainGluePlacedModulesTest::RunTest(const FString& Parameters)
{
	using namespace ShipBuilderDomainGlueChainSocketsTestPrivate;

	FTestResolver Resolver;
	UShipModuleDefinition* Lower = MakeDefinition(
		TEXT("PlacedLower"),
		EShipModuleType::Corridor,
		{ EShipModuleType::Corridor });

	UShipModuleDefinition* Upper = MakeDefinition(
		TEXT("PlacedUpper"),
		EShipModuleType::Corridor,
		{ EShipModuleType::Corridor });

	Resolver.Add(Lower);
	Resolver.Add(Upper);

	FShipBuilderDraftConfig Draft;
	FShipBuilderDraftConfig::FPlacedModule A;
	A.InstanceId = TEXT("A");
	A.ModuleId = Lower->ModuleId;
	A.GridPos = FIntVector(0, 0, 0);
	A.YawStep = 0;
	Draft.PlacedModules.Add(A);
	FShipBuilderDraftConfig::FPlacedModule B;
	B.InstanceId = TEXT("B");
	B.ModuleId = Upper->ModuleId;
	B.GridPos = FIntVector(0, 0, 1);
	B.YawStep = 0;
	Draft.PlacedModules.Add(B);
	FShipBuilderDraftConfig::FConnection Link;
	Link.ModuleAInstanceId = TEXT("A");
	Link.ModuleASocketName = TEXT("Top");
	Link.ModuleBInstanceId = TEXT("B");
	Link.ModuleBSocketName = TEXT("Bottom");
	Draft.Connections.Add(Link);

	FShipBuildDomainModel Model(Resolver);
	FString Error;
	const bool bBuilt = SpaceshipCrew_BuildDomainFromDraftChain(Draft, Resolver, Model, Error);
	TestTrue(FString::Printf(TEXT("BuildPlacedModules: %s"), *Error), bBuilt);
	TestTrue(TEXT("PlacedModulesValidation"), Model.Validate().bIsValid);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShipBuilderDomainGlueRotatedSocketTest,
	"SpaceshipCrew.ShipBuilder.DomainGlue.RotatedSocketForGridDelta",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FShipBuilderDomainGlueRotatedSocketTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Yaw0Front"), SpaceshipCrew_LocalSocketForGridDelta(FIntVector(1, 0, 0), 0), FName(TEXT("Front")));
	TestEqual(TEXT("Yaw0Back"), SpaceshipCrew_LocalSocketForGridDelta(FIntVector(-1, 0, 0), 0), FName(TEXT("Back")));
	TestEqual(TEXT("Yaw0Right"), SpaceshipCrew_LocalSocketForGridDelta(FIntVector(0, 1, 0), 0), FName(TEXT("Right")));
	TestEqual(TEXT("Yaw0Left"), SpaceshipCrew_LocalSocketForGridDelta(FIntVector(0, -1, 0), 0), FName(TEXT("Left")));

	// +90 yaw: local Right faces world +X.
	TestEqual(TEXT("Yaw1WorldPlusX"), SpaceshipCrew_LocalSocketForGridDelta(FIntVector(1, 0, 0), 1), FName(TEXT("Right")));
	TestEqual(TEXT("Yaw1WorldPlusY"), SpaceshipCrew_LocalSocketForGridDelta(FIntVector(0, 1, 0), 1), FName(TEXT("Front")));
	TestEqual(TEXT("Yaw2WorldPlusX"), SpaceshipCrew_LocalSocketForGridDelta(FIntVector(1, 0, 0), 2), FName(TEXT("Back")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShipBuilderDomainGlueSocketSnapTest,
	"SpaceshipCrew.ShipBuilder.DomainGlue.SocketSnapAndDock",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FShipBuilderDomainGlueSocketSnapTest::RunTest(const FString& Parameters)
{
	using namespace ShipBuilderDomainGlueChainSocketsTestPrivate;

	FTestResolver Resolver;
	UShipModuleDefinition* Corridor = MakeDefinition(
		TEXT("SnapCorridor"),
		EShipModuleType::Corridor,
		{ EShipModuleType::Corridor });
	Resolver.Add(Corridor);

	FShipBuilderDraftConfig Draft;
	FShipBuilderDraftConfig::FPlacedModule Anchor;
	Anchor.InstanceId = TEXT("Anchor");
	Anchor.ModuleId = Corridor->ModuleId;
	Anchor.GridPos = FIntVector(0, 0, 0);
	Anchor.YawStep = 0;
	Draft.PlacedModules.Add(Anchor);

	FShipBuilderDraftConfig::FPlacedModule Moving;
	Moving.InstanceId = TEXT("Moving");
	Moving.ModuleId = Corridor->ModuleId;
	Moving.YawStep = 0;
	Moving.GridPos = FIntVector(0, 0, 0);
	Draft.PlacedModules.Add(Moving);

	FIntVector BestCell = Moving.GridPos;
	const bool bFound = SpaceshipCrew_TryFindBestSocketSnapCell(
		Draft,
		Moving,
		*Corridor,
		FIntVector(0, 0, 0),
		Moving.InstanceId,
		[&Resolver](const FName ModuleId) { return Resolver.ResolveModule(ModuleId); },
		BestCell,
		400.0f,
		300.0f,
		900.0f);
	TestTrue(TEXT("SocketSnapFound"), bFound);
	TestNotEqual(TEXT("SnappedOffOverlapCell"), BestCell, FIntVector(0, 0, 0));

	Moving.GridPos = BestCell;
	const FShipBuilderModuleWorldPlacement AnchorPlacement = SpaceshipCrew_BuildModuleWorldPlacement(Anchor, *Corridor);
	const FShipBuilderModuleWorldPlacement MovingPlacement = SpaceshipCrew_BuildModuleWorldPlacement(Moving, *Corridor);
	FName SocketA = NAME_None;
	FName SocketB = NAME_None;
	TestTrue(
		TEXT("DockDetected"),
		SpaceshipCrew_TryGetSocketDockingBetweenPlacedModules(
			AnchorPlacement,
			*Corridor,
			MovingPlacement,
			*Corridor,
			SocketA,
			SocketB));
	TestEqual(TEXT("AnchorFrontSocket"), SocketA, FName(TEXT("Front_X0_Y0_Z0")));
	TestEqual(TEXT("MovingBackSocket"), SocketB, FName(TEXT("Back_X0_Y0_Z0")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShipBuilderDomainGluePartialSocketSnapTest,
	"SpaceshipCrew.ShipBuilder.DomainGlue.PartialSocketOverrideSnap",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FShipBuilderDomainGluePartialSocketSnapTest::RunTest(const FString& Parameters)
{
	using namespace ShipBuilderDomainGlueChainSocketsTestPrivate;

	FTestResolver Resolver;
	UShipModuleDefinition* Corridor = MakeDefinition(
		TEXT("PartialSnapCorridor"),
		EShipModuleType::Corridor,
		{ EShipModuleType::Corridor });

	UShipModuleVisualOverride* PartialOverride = NewObject<UShipModuleVisualOverride>();
	FShipModuleContactPoint TopOnly;
	TopOnly.SocketName = TEXT("Top");
	TopOnly.RelativeLocation = FVector(0.0f, 0.0f, 150.0f);
	TopOnly.SocketType = EShipModuleSocketType::Vertical;
	PartialOverride->bOverrideContactPoints = true;
	PartialOverride->ContactPointsOverride.Add(TopOnly);
	Corridor->VisualOverride = PartialOverride;

	TArray<FShipModuleContactPoint> PlacementSockets;
	Corridor->GatherContactPointsForPlacement(PlacementSockets);
	TestTrue(TEXT("PartialOverrideIncludesDefaults"), PlacementSockets.Num() >= 5);

	Resolver.Add(Corridor);

	FShipBuilderDraftConfig Draft;
	FShipBuilderDraftConfig::FPlacedModule Anchor;
	Anchor.InstanceId = TEXT("Anchor");
	Anchor.ModuleId = Corridor->ModuleId;
	Anchor.GridPos = FIntVector(1, 0, 0);
	Anchor.YawStep = 0;
	Draft.PlacedModules.Add(Anchor);

	FShipBuilderDraftConfig::FPlacedModule Moving;
	Moving.InstanceId = TEXT("Moving");
	Moving.ModuleId = Corridor->ModuleId;
	Moving.YawStep = 0;
	Moving.GridPos = FIntVector(-1, 0, 0);
	Draft.PlacedModules.Add(Moving);

	FIntVector BestCell = Moving.GridPos;
	const bool bFound = SpaceshipCrew_TryFindBestSocketSnapCell(
		Draft,
		Moving,
		*Corridor,
		FIntVector(0, 0, 0),
		Moving.InstanceId,
		[&Resolver](const FName ModuleId) { return Resolver.ResolveModule(ModuleId); },
		BestCell,
		400.0f,
		300.0f,
		900.0f);
	TestTrue(TEXT("PartialOverrideSnapFound"), bFound);
	TestEqual(TEXT("SnappedToNeighborCell"), BestCell, FIntVector(0, 0, 0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShipBuilderDomainGlueRotatedDockTest,
	"SpaceshipCrew.ShipBuilder.DomainGlue.RotatedGridDock",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FShipBuilderDomainGlueRotatedDockTest::RunTest(const FString& Parameters)
{
	using namespace ShipBuilderDomainGlueChainSocketsTestPrivate;

	FTestResolver Resolver;
	UShipModuleDefinition* Corridor = MakeDefinition(
		TEXT("RotatedDockCorridor"),
		EShipModuleType::Corridor,
		{ EShipModuleType::Corridor });
	Resolver.Add(Corridor);

	FShipBuilderPlacedModule Anchor;
	Anchor.InstanceId = TEXT("Anchor");
	Anchor.ModuleId = Corridor->ModuleId;
	Anchor.GridPos = FIntVector(0, 0, 0);
	Anchor.YawStep = 0;

	FShipBuilderPlacedModule Moving;
	Moving.InstanceId = TEXT("Moving");
	Moving.ModuleId = Corridor->ModuleId;
	Moving.GridPos = FIntVector(1, 0, 0);
	Moving.YawStep = 1;

	const FShipBuilderModuleWorldPlacement AnchorPlacement = SpaceshipCrew_BuildModuleWorldPlacement(Anchor, *Corridor);
	const FShipBuilderModuleWorldPlacement MovingPlacement = SpaceshipCrew_BuildModuleWorldPlacement(Moving, *Corridor);
	FName SocketA = NAME_None;
	FName SocketB = NAME_None;
	TestTrue(
		TEXT("RotatedDockDetected"),
		SpaceshipCrew_TryGetSocketDockingBetweenPlacedModules(
			AnchorPlacement,
			*Corridor,
			MovingPlacement,
			*Corridor,
			SocketA,
			SocketB));
	TestEqual(TEXT("AnchorFrontSocket"), SocketA, FName(TEXT("Front_X0_Y0_Z0")));
	TestEqual(TEXT("MovingLeftSocket"), SocketB, FName(TEXT("Left_X0_Y0_Z0")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShipBuilderDomainGlueMixedSizeSnapTest,
	"SpaceshipCrew.ShipBuilder.DomainGlue.MixedSizeSocketSnap",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FShipBuilderDomainGlueMixedSizeSnapTest::RunTest(const FString& Parameters)
{
	using namespace ShipBuilderDomainGlueChainSocketsTestPrivate;

	FTestResolver Resolver;
	UShipModuleDefinition* Standard = MakeDefinition(
		TEXT("MixedStandard"),
		EShipModuleType::Corridor,
		{ EShipModuleType::Corridor });

	UShipModuleDefinition* Wide = NewObject<UShipModuleDefinition>();
	Wide->ModuleId = TEXT("MixedWide");
	Wide->ModuleType = EShipModuleType::Corridor;
	Wide->DisplayName = FText::FromString(TEXT("MixedWide"));
	Wide->Mass = 100.0f;
	Wide->Size = FVector(500.0, 800.0, 300.0);
	Wide->CompatibleModuleTypes = { EShipModuleType::Corridor };
	Wide->SyncCellSizeAndSizeFromLegacy();
	Wide->EnsureContactPointsPopulatedIfNoAuthoringOverride();
	TestEqual(TEXT("WideCellSizeX"), Wide->GetEffectiveCellSize().X, 1);
	TestEqual(TEXT("WideCellSizeY"), Wide->GetEffectiveCellSize().Y, 2);
	TestEqual(TEXT("WideCellSizeZ"), Wide->GetEffectiveCellSize().Z, 1);

	Resolver.Add(Standard);
	Resolver.Add(Wide);

	FShipBuilderDraftConfig Draft;
	FShipBuilderDraftConfig::FPlacedModule Anchor;
	Anchor.InstanceId = TEXT("Anchor");
	Anchor.ModuleId = Standard->ModuleId;
	Anchor.GridPos = FIntVector(0, 0, 0);
	Anchor.YawStep = 0;
	Draft.PlacedModules.Add(Anchor);

	FShipBuilderDraftConfig::FPlacedModule Moving;
	Moving.InstanceId = TEXT("Moving");
	Moving.ModuleId = Wide->ModuleId;
	Moving.GridPos = FIntVector(2, 0, 0);
	Moving.YawStep = 0;
	Draft.PlacedModules.Add(Moving);

	FIntVector BestCell = Moving.GridPos;
	const bool bFound = SpaceshipCrew_TryFindBestSocketSnapCell(
		Draft,
		Moving,
		*Wide,
		FIntVector(1, 0, 0),
		Moving.InstanceId,
		[&Resolver](const FName ModuleId) { return Resolver.ResolveModule(ModuleId); },
		BestCell,
		400.0f,
		300.0f,
		1200.0f);
	TestTrue(TEXT("MixedSizeSnapFound"), bFound);
	TestEqual(TEXT("SnappedCorner"), BestCell, FIntVector(1, 0, 0));

	Moving.GridPos = BestCell;
	const FShipBuilderModuleWorldPlacement AnchorPlacement = SpaceshipCrew_BuildModuleWorldPlacement(Anchor, *Standard);
	const FShipBuilderModuleWorldPlacement MovingPlacement = SpaceshipCrew_BuildModuleWorldPlacement(Moving, *Wide);
	FName SocketA = NAME_None;
	FName SocketB = NAME_None;
	TestTrue(
		TEXT("MixedSizeDockDetected"),
		SpaceshipCrew_TryGetSocketDockingBetweenPlacedModules(
			AnchorPlacement,
			*Standard,
			MovingPlacement,
			*Wide,
			SocketA,
			SocketB));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShipBuilderDomainGlueLShapePanelSnapTest,
	"SpaceshipCrew.ShipBuilder.DomainGlue.LShapePanelSnap",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FShipBuilderDomainGlueLShapePanelSnapTest::RunTest(const FString& Parameters)
{
	using namespace ShipBuilderDomainGlueChainSocketsTestPrivate;

	FTestResolver Resolver;

	UShipModuleDefinition* Tall = NewObject<UShipModuleDefinition>();
	Tall->ModuleId = TEXT("TallCorridor");
	Tall->ModuleType = EShipModuleType::Corridor;
	Tall->DisplayName = FText::FromString(TEXT("TallCorridor"));
	Tall->Mass = 100.0f;
	Tall->Size = FVector(400.0, 800.0, 300.0);
	Tall->CompatibleModuleTypes = { EShipModuleType::Corridor };
	Tall->SyncCellSizeAndSizeFromLegacy();

	UShipModuleDefinition* Small = MakeDefinition(
		TEXT("SmallCorridor"),
		EShipModuleType::Corridor,
		{ EShipModuleType::Corridor });

	Resolver.Add(Tall);
	Resolver.Add(Small);
	TestEqual(TEXT("TallCellSizeY"), Tall->GetEffectiveCellSize().Y, 2);

	FShipBuilderDraftConfig Draft;
	FShipBuilderPlacedModule Anchor;
	Anchor.InstanceId = TEXT("Tall");
	Anchor.ModuleId = Tall->ModuleId;
	Anchor.GridPos = FIntVector(0, 0, 0);
	Anchor.YawStep = 0;
	Draft.PlacedModules.Add(Anchor);

	FShipBuilderPlacedModule Moving;
	Moving.InstanceId = TEXT("Small");
	Moving.ModuleId = Small->ModuleId;
	Moving.GridPos = FIntVector(2, 0, 0);
	Moving.YawStep = 0;
	Draft.PlacedModules.Add(Moving);

	FIntVector BestCell = Moving.GridPos;
	const bool bFound = SpaceshipCrew_TryFindBestSocketSnapCell(
		Draft,
		Moving,
		*Small,
		FIntVector(1, 0, 0),
		Moving.InstanceId,
		[&Resolver](const FName ModuleId) { return Resolver.ResolveModule(ModuleId); },
		BestCell,
		400.0f,
		300.0f,
		1200.0f);
	TestTrue(TEXT("LShapeSnapFound"), bFound);
	TestEqual(TEXT("LShapeCornerCell"), BestCell, FIntVector(1, 0, 0));

	Moving.GridPos = BestCell;
	const FShipBuilderModuleWorldPlacement TallPlacement = SpaceshipCrew_BuildModuleWorldPlacement(Anchor, *Tall);
	const FShipBuilderModuleWorldPlacement SmallPlacement = SpaceshipCrew_BuildModuleWorldPlacement(Moving, *Small);
	FName SocketA = NAME_None;
	FName SocketB = NAME_None;
	TestTrue(
		TEXT("LShapeDockDetected"),
		SpaceshipCrew_TryGetSocketDockingBetweenPlacedModules(
			TallPlacement,
			*Tall,
			SmallPlacement,
			*Small,
			SocketA,
			SocketB));
	TestEqual(TEXT("TallLowerFrontPanel"), SocketA, FName(TEXT("Front_X0_Y0_Z0")));
	TestEqual(TEXT("SmallBackPanel"), SocketB, FName(TEXT("Back_X0_Y0_Z0")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShipBuilderDomainGlueBilateralConnectionTest,
	"SpaceshipCrew.ShipBuilder.DomainGlue.BilateralConnectionRebuild",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FShipBuilderDomainGlueBilateralConnectionTest::RunTest(const FString& Parameters)
{
	using namespace ShipBuilderDomainGlueChainSocketsTestPrivate;

	FTestResolver Resolver;
	UShipModuleDefinition* Corridor = MakeDefinition(
		TEXT("BilateralCorridor"),
		EShipModuleType::Corridor,
		{ EShipModuleType::Corridor });
	Resolver.Add(Corridor);

	FShipBuilderDraftConfig Draft;
	FShipBuilderPlacedModule Left;
	Left.InstanceId = TEXT("Left");
	Left.ModuleId = Corridor->ModuleId;
	Left.GridPos = FIntVector(0, 0, 0);
	Left.YawStep = 0;
	Draft.PlacedModules.Add(Left);

	FShipBuilderPlacedModule Right;
	Right.InstanceId = TEXT("Right");
	Right.ModuleId = Corridor->ModuleId;
	Right.GridPos = FIntVector(1, 0, 0);
	Right.YawStep = 0;
	Draft.PlacedModules.Add(Right);

	SpaceshipCrew_RebuildDraftConnectionsFromAdjacency(
		Draft,
		[&Resolver](const FName ModuleId) { return Resolver.ResolveModule(ModuleId); },
		400.0f,
		300.0f);

	TestEqual(TEXT("SingleConnection"), Draft.Connections.Num(), 1);

	const FShipBuilderDraftConnection& Link = Draft.Connections[0];
	const bool bLeftIsA = Link.ModuleAInstanceId == Left.InstanceId;
	const FName LeftSocket = bLeftIsA ? Link.ModuleASocketName : Link.ModuleBSocketName;
	const FName RightSocket = bLeftIsA ? Link.ModuleBSocketName : Link.ModuleASocketName;

	TestTrue(TEXT("LeftHasFrontPanel"), LeftSocket.ToString().StartsWith(TEXT("Front_")));
	TestTrue(TEXT("RightHasBackPanel"), RightSocket.ToString().StartsWith(TEXT("Back_")));
	TestTrue(
		TEXT("LeftPanelMatchesConnection"),
		SpaceshipCrew_DoPanelSocketsMatch(LeftSocket, LeftSocket));
	TestTrue(
		TEXT("RightPanelMatchesConnection"),
		SpaceshipCrew_DoPanelSocketsMatch(RightSocket, RightSocket));
	return true;
}

#endif
