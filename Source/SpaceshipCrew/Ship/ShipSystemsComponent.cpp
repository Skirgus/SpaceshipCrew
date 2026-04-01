// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShipSystemsComponent.h"
#include "SpaceshipCrew.h"
#include "CrewRoleComponent.h"
#include "ShipConfigAsset.h"
#include "ShipModuleDefinition.h"
#include "ShipInteractableBase.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Containers/Queue.h"

UShipSystemsComponent::UShipSystemsComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void UShipSystemsComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UShipSystemsComponent, ReactorOutput);
	DOREPLIFETIME(UShipSystemsComponent, OxygenLevel);
	DOREPLIFETIME(UShipSystemsComponent, OxygenReserve);
	DOREPLIFETIME(UShipSystemsComponent, FuelReserve);
	DOREPLIFETIME(UShipSystemsComponent, bOxygenSupplyEnabled);
	DOREPLIFETIME(UShipSystemsComponent, HullIntegrity);
	DOREPLIFETIME(UShipSystemsComponent, FireIntensity);
	DOREPLIFETIME(UShipSystemsComponent, FireHotspotCount);
	DOREPLIFETIME(UShipSystemsComponent, Compartments);
	DOREPLIFETIME(UShipSystemsComponent, Bulkheads);
	DOREPLIFETIME(UShipSystemsComponent, ActiveShipConfigId);
	DOREPLIFETIME(UShipSystemsComponent, LastConfigValidationErrors);
	DOREPLIFETIME(UShipSystemsComponent, RecentAlerts);
}

void UShipSystemsComponent::OnRep_Snapshot()
{
	OnBulkheadsChanged.Broadcast();
}

void UShipSystemsComponent::OnRep_RecentAlerts()
{
	if (RecentAlerts.Num() > 0)
	{
		OnShipAlert.Broadcast(RecentAlerts.Last());
	}
}

void UShipSystemsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		SimulateSystems(DeltaTime);
	}
}

void UShipSystemsComponent::BeginPlay()
{
	Super::BeginPlay();
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		InitializeDefaultModuleLayout();
		TArray<FText> Errors;
		ValidateCurrentSetup(Errors);
		LastConfigValidationErrors = Errors;
		RebuildGlobalSnapshotFromModules();
	}
}

void UShipSystemsComponent::SimulateSystems(float DeltaTime)
{
	SimulateModuleCompartments(DeltaTime);
	EvaluateReactorFireRisk(DeltaTime);

	if (FireHotspotCount <= 0 && ActiveFireStations.Num() > 0)
	{
		for (TObjectPtr<AShipInteractableBase>& Station : ActiveFireStations)
		{
			if (IsValid(Station))
			{
				Station->Destroy();
			}
		}
		ActiveFireStations.Empty();
		PushAlert(FText::FromString(TEXT("Fire contained near reactor")), EShipAlertSeverity::Info);
	}

	if (OxygenLevel < 8.f)
	{
		HullIntegrity = FMath::Clamp(HullIntegrity - 0.01f * DeltaTime, 0.f, 1.f);
	}
}

void UShipSystemsComponent::EvaluateReactorFireRisk(float DeltaTime)
{
	const float Output = ReactorOutput;
	const bool bOverloaded = (Output > 1.0f);

	const int32 ReactorIndex = FindFirstModuleIndexByType(EShipModuleType::Reactor);
	const float ReactorOxygen = (ReactorIndex != INDEX_NONE) ? Compartments[ReactorIndex].OxygenLevel : OxygenLevel;

	// При низком O2 в реакторном отсеке новые очаги пожара не возникают.
	if (ReactorOxygen <= MinOxygenPercentForNewFires)
	{
		OverloadGuaranteeProgress = 0.f;
		bLastOverloadState = false;
		if (!bLowOxygenFireSuppressionAlerted)
		{
			PushAlert(
				FText::Format(
					FText::FromString(TEXT("Oxygen {0}% <= {1}%: new fire hotspots are suppressed")),
					FText::AsNumber(FMath::RoundToInt(ReactorOxygen)),
					FText::AsNumber(FMath::RoundToInt(MinOxygenPercentForNewFires))
				),
				EShipAlertSeverity::Info
			);
			bLowOxygenFireSuppressionAlerted = true;
		}
		return;
	}
	bLowOxygenFireSuppressionAlerted = false;

	if (Output <= 0.75f)
	{
		OverloadGuaranteeProgress = 0.f;
		bLastOverloadState = false;
		return;
	}

	// 75..100%: растущая вероятность, но без гарантии.
	if (Output <= 1.0f)
	{
		OverloadGuaranteeProgress = 0.f;
		bLastOverloadState = false;
		const float Alpha = FMath::GetMappedRangeValueClamped(FVector2D(0.75f, 1.0f), FVector2D(0.f, 1.f), Output);
		const float ChancePerSecond = FMath::Lerp(0.0f, 0.18f, Alpha);
		if (FMath::FRand() < ChancePerSecond * DeltaTime)
		{
			TriggerReactorFire(FText::FromString(TEXT("Reactor heat spike ignited a local fire")), EShipAlertSeverity::Warning);
		}
		return;
	}

	// >100%: пожар гарантирован; ближе к 120% возникает быстрее.
	if (!bLastOverloadState)
	{
		PushAlert(FText::FromString(TEXT("Reactor overload started (>100%)")), EShipAlertSeverity::Warning);
	}
	bLastOverloadState = bOverloaded;

	const float SecondsToGuaranteedFire = FMath::GetMappedRangeValueClamped(
		FVector2D(1.0f, 1.2f), FVector2D(10.0f, 1.2f), Output);
	OverloadGuaranteeProgress += DeltaTime / FMath::Max(0.1f, SecondsToGuaranteedFire);
	if (OverloadGuaranteeProgress >= 1.0f)
	{
		OverloadGuaranteeProgress = 0.f;
		TriggerReactorFire(FText::FromString(TEXT("Reactor overload caused guaranteed fire")), EShipAlertSeverity::Critical);
	}
}

void UShipSystemsComponent::TriggerReactorFire(const FText& Reason, EShipAlertSeverity Severity)
{
	const int32 ReactorIndex = FindFirstModuleIndexByType(EShipModuleType::Reactor);
	if (ReactorIndex != INDEX_NONE && Compartments[ReactorIndex].FireHotspotCount < MaxFireHotspots)
	{
		++Compartments[ReactorIndex].FireHotspotCount;
		Compartments[ReactorIndex].FireIntensity = FMath::Clamp(
			static_cast<float>(Compartments[ReactorIndex].FireHotspotCount) / FMath::Max(1, MaxFireHotspots),
			0.f,
			1.f
		);
		RebuildGlobalSnapshotFromModules();
		SpawnOneFireStation();
	}
	PushAlert(Reason, Severity);
	PushAlert(
		FText::Format(
			FText::FromString(TEXT("Fire hotspot count: {0}/{1}")),
			FText::AsNumber(FireHotspotCount),
			FText::AsNumber(MaxFireHotspots)
		),
		EShipAlertSeverity::Warning
	);
}

void UShipSystemsComponent::SpawnOneFireStation()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	if (!FireStationClass)
	{
		PushAlert(FText::FromString(TEXT("FireStationClass is not configured on ShipSystemsComponent")), EShipAlertSeverity::Warning);
		return;
	}

	const FVector ReactorCenter = GetOwner()->GetActorTransform().TransformPosition(ReactorWorldOffset);
	FVector BoundsOrigin = FVector::ZeroVector;
	FVector BoundsExtent = FVector::ZeroVector;
	GetOwner()->GetActorBounds(true, BoundsOrigin, BoundsExtent);

	const auto IsInsideShipBounds = [&BoundsOrigin, &BoundsExtent](const FVector& P) -> bool
	{
		const FBox ShipBox(BoundsOrigin - BoundsExtent, BoundsOrigin + BoundsExtent);
		return ShipBox.IsInsideOrOn(P);
	};
	const auto IsFarEnoughFromExistingStations = [this](const FVector& P) -> bool
	{
		constexpr float MinSpacing = 140.f;
		for (const TObjectPtr<AShipInteractableBase>& Station : ActiveFireStations)
		{
			if (!IsValid(Station))
			{
				continue;
			}
			if (FVector::DistSquared(Station->GetActorLocation(), P) < FMath::Square(MinSpacing))
			{
				return false;
			}
		}
		return true;
	};

	const float MinRadius = FMath::Min(50.f, FireStationSpawnRadius);
	FVector SpawnLocation = ReactorCenter;
	bool bFoundInsidePoint = false;

	// Сначала пытаемся найти случайную точку около реактора, но строго внутри bounds корабля.
	// Для Z ищем поверхность пола через trace и поднимаем станцию над ней.
	for (int32 Attempt = 0; Attempt < 12; ++Attempt)
	{
		const FVector RandDir = FMath::VRand();
		const float Radius = FMath::FRandRange(MinRadius, FireStationSpawnRadius);

		const FVector CandidateXY = ReactorCenter + FVector(RandDir.X, RandDir.Y, 0.f) * Radius;
		const float TraceTopZ = BoundsOrigin.Z + BoundsExtent.Z + 20.f;
		const float TraceBottomZ = BoundsOrigin.Z - BoundsExtent.Z - 20.f;
		const FVector TraceStart(CandidateXY.X, CandidateXY.Y, TraceTopZ);
		const FVector TraceEnd(CandidateXY.X, CandidateXY.Y, TraceBottomZ);

		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(FireStationFloorTrace), true, GetOwner());
		const bool bHitFloor = GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params);

		FVector Candidate = CandidateXY;
		if (bHitFloor)
		{
			// Поднимаем станцию над полом, чтобы её можно было навести и активировать.
			Candidate.Z = Hit.ImpactPoint.Z + 45.f;
		}
		else
		{
			// Fallback: если пол не найден, ставим немного выше центра реактора.
			Candidate.Z = ReactorCenter.Z + 30.f;
		}

		if (IsInsideShipBounds(Candidate) && IsFarEnoughFromExistingStations(Candidate))
		{
			SpawnLocation = Candidate;
			bFoundInsidePoint = true;
			break;
		}
	}

	// Fallback: гарантированно ставим СЛУЧАЙНУЮ точку внутри bounds корабля,
	// чтобы очаги не "залипали" в одном месте.
	if (!bFoundInsidePoint)
	{
		const FVector Margin(30.f, 30.f, 10.f);
		const FVector MinP = BoundsOrigin - BoundsExtent + Margin;
		const FVector MaxP = BoundsOrigin + BoundsExtent - Margin;

		// Пробуем несколько случайных точек внутри bounds с разносом от уже активных станций.
		for (int32 Attempt = 0; Attempt < 20; ++Attempt)
		{
			FVector Candidate;
			Candidate.X = FMath::FRandRange(MinP.X, MaxP.X);
			Candidate.Y = FMath::FRandRange(MinP.Y, MaxP.Y);
			Candidate.Z = FMath::FRandRange(MinP.Z, MaxP.Z);

			// Привязываем к полу через vertical trace.
			const FVector TraceStart(Candidate.X, Candidate.Y, MaxP.Z + 40.f);
			const FVector TraceEnd(Candidate.X, Candidate.Y, MinP.Z - 40.f);
			FHitResult Hit;
			FCollisionQueryParams Params(SCENE_QUERY_STAT(FireStationFallbackFloorTrace), true, GetOwner());
			if (GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params))
			{
				Candidate.Z = Hit.ImpactPoint.Z + 45.f;
			}

			if (IsInsideShipBounds(Candidate) && IsFarEnoughFromExistingStations(Candidate))
			{
				SpawnLocation = Candidate;
				bFoundInsidePoint = true;
				break;
			}
		}

		// Последняя защита: даже если не нашли "идеал", слегка разнесём от центра.
		if (!bFoundInsidePoint)
		{
			const FVector Jitter(
				FMath::FRandRange(-120.f, 120.f),
				FMath::FRandRange(-120.f, 120.f),
				0.f
			);
			SpawnLocation = FVector(
				FMath::Clamp(BoundsOrigin.X + Jitter.X, MinP.X, MaxP.X),
				FMath::Clamp(BoundsOrigin.Y + Jitter.Y, MinP.Y, MaxP.Y),
				FMath::Clamp(BoundsOrigin.Z - BoundsExtent.Z + 55.f, MinP.Z, MaxP.Z)
			);
		}
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	Params.Owner = GetOwner();

	AShipInteractableBase* Spawned = GetWorld()->SpawnActor<AShipInteractableBase>(
		FireStationClass,
		SpawnLocation,
		GetOwner()->GetActorRotation(),
		Params
	);
	if (!Spawned)
	{
		PushAlert(FText::FromString(TEXT("Failed to spawn fire station actor")), EShipAlertSeverity::Warning);
		return;
	}

	Spawned->OwningShipActor = GetOwner();
	Spawned->RequiredPermission = FName(TEXT("Extinguisher"));
	Spawned->ActionId = FName(TEXT("FightFire"));
	Spawned->Magnitude = 1.f;
	ActiveFireStations.Add(Spawned);
}

void UShipSystemsComponent::RemoveOneFireHotspot(APawn* InstigatorPawn)
{
	if (FireHotspotCount <= 0)
	{
		return;
	}
	int32 ModuleIndexToReduce = FindFirstModuleIndexByType(EShipModuleType::Reactor);
	if (ModuleIndexToReduce == INDEX_NONE)
	{
		for (int32 i = 0; i < Compartments.Num(); ++i)
		{
			if (Compartments[i].FireHotspotCount > 0)
			{
				ModuleIndexToReduce = i;
				break;
			}
		}
	}
	if (ModuleIndexToReduce != INDEX_NONE && Compartments[ModuleIndexToReduce].FireHotspotCount > 0)
	{
		--Compartments[ModuleIndexToReduce].FireHotspotCount;
		Compartments[ModuleIndexToReduce].FireIntensity = FMath::Clamp(
			static_cast<float>(Compartments[ModuleIndexToReduce].FireHotspotCount) / FMath::Max(1, MaxFireHotspots),
			0.f,
			1.f
		);
	}
	SyncFireIntensityFromHotspots();

	AShipInteractableBase* BestStation = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();
	const FVector RefLoc = InstigatorPawn ? InstigatorPawn->GetActorLocation() : FVector::ZeroVector;

	for (TObjectPtr<AShipInteractableBase>& Station : ActiveFireStations)
	{
		if (!IsValid(Station))
		{
			continue;
		}
		const float DistSq = InstigatorPawn
			? FVector::DistSquared(Station->GetActorLocation(), RefLoc)
			: 0.f;
		if (!BestStation || DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			BestStation = Station;
		}
	}

	if (IsValid(BestStation))
	{
		BestStation->Destroy();
	}

	ActiveFireStations.RemoveAll([](const TObjectPtr<AShipInteractableBase>& Station)
	{
		return !IsValid(Station);
	});
}

void UShipSystemsComponent::SyncFireIntensityFromHotspots()
{
	RebuildGlobalSnapshotFromModules();
}

void UShipSystemsComponent::InitializeDefaultModuleLayout()
{
	if (Compartments.Num() == 0)
	{
		FShipCompartmentState Reactor;
		Reactor.ModuleId = FName(TEXT("Reactor"));
		Reactor.ModuleType = EShipModuleType::Reactor;
		Reactor.bMandatory = true;
		Reactor.bProvidesFuelStorage = true;
		Compartments.Add(Reactor);

		FShipCompartmentState Engine;
		Engine.ModuleId = FName(TEXT("Engine"));
		Engine.ModuleType = EShipModuleType::Engine;
		Engine.bMandatory = true;
		Compartments.Add(Engine);

		FShipCompartmentState Bridge;
		Bridge.ModuleId = FName(TEXT("Bridge"));
		Bridge.ModuleType = EShipModuleType::Bridge;
		Bridge.bMandatory = true;
		Bridge.bProvidesOxygenStorage = true;
		Compartments.Add(Bridge);

		FShipCompartmentState Airlock;
		Airlock.ModuleId = FName(TEXT("Airlock"));
		Airlock.ModuleType = EShipModuleType::Airlock;
		Airlock.bMandatory = true;
		Compartments.Add(Airlock);
	}

	if (Bulkheads.Num() == 0)
	{
		FShipBulkheadState AB;
		AB.ModuleA = FName(TEXT("Reactor"));
		AB.ModuleB = FName(TEXT("Engine"));
		Bulkheads.Add(AB);

		FShipBulkheadState BC;
		BC.ModuleA = FName(TEXT("Engine"));
		BC.ModuleB = FName(TEXT("Bridge"));
		Bulkheads.Add(BC);

		FShipBulkheadState CA;
		CA.ModuleA = FName(TEXT("Bridge"));
		CA.ModuleB = FName(TEXT("Airlock"));
		Bulkheads.Add(CA);
	}
}

void UShipSystemsComponent::SimulateModuleCompartments(float DeltaTime)
{
	if (Compartments.Num() == 0)
	{
		InitializeDefaultModuleLayout();
	}

	const float FireOxygenBurnRate = 3.0f;
	const float BreachOxygenLeakRate = 7.0f;
	const float RefillRate = 4.0f;
	bool bAnyReserveDepletedThisTick = false;

	for (int32 i = 0; i < Compartments.Num(); ++i)
	{
		FShipCompartmentState& Module = Compartments[i];
		Module.FireIntensity = FMath::Clamp(static_cast<float>(Module.FireHotspotCount) / FMath::Max(1, MaxFireHotspots), 0.f, 1.f);

		if (Module.FireIntensity > 0.01f)
		{
			Module.OxygenLevel = FMath::Clamp(Module.OxygenLevel - FireOxygenBurnRate * Module.FireIntensity * DeltaTime, 0.f, TargetOxygenPercent);
		}

		if (Module.BreachSeverity > 0.01f)
		{
			Module.OxygenLevel = FMath::Clamp(Module.OxygenLevel - BreachOxygenLeakRate * Module.BreachSeverity * DeltaTime, 0.f, TargetOxygenPercent);
		}

		if (bOxygenSupplyEnabled && OxygenReserve > 0.f && IsModuleConnectedToOxygenSource(i) && Module.OxygenLevel < TargetOxygenPercent)
		{
			const float Need = TargetOxygenPercent - Module.OxygenLevel;
			const float Add = FMath::Min3(Need, RefillRate * DeltaTime, OxygenReserve);
			Module.OxygenLevel = FMath::Clamp(Module.OxygenLevel + Add, 0.f, TargetOxygenPercent);
			OxygenReserve = FMath::Max(0.f, OxygenReserve - Add);
			if (OxygenReserve <= 0.01f)
			{
				bAnyReserveDepletedThisTick = true;
			}
		}

		if (Module.OxygenLevel <= MinOxygenPercentForNewFires && Module.FireHotspotCount > 0)
		{
			const float SecondsPerHotspot = FMath::GetMappedRangeValueClamped(
				FVector2D(MinOxygenPercentForNewFires, 0.f),
				FVector2D(LowOxygenExtinguishSecondsAtThreshold, LowOxygenExtinguishSecondsAtZero),
				Module.OxygenLevel
			);
			Module.LowOxygenExtinguishProgress += DeltaTime / FMath::Max(0.2f, SecondsPerHotspot);
			while (Module.LowOxygenExtinguishProgress >= 1.f && Module.FireHotspotCount > 0)
			{
				Module.LowOxygenExtinguishProgress -= 1.f;
				--Module.FireHotspotCount;
				PushAlert(FText::Format(FText::FromString(TEXT("Fire hotspot extinguished by low oxygen in {0}")), FText::FromName(Module.ModuleId)), EShipAlertSeverity::Info);
			}
		}
		else
		{
			Module.LowOxygenExtinguishProgress = 0.f;
		}
	}

	if (bAnyReserveDepletedThisTick)
	{
		bOxygenSupplyEnabled = false;
		PushAlert(FText::FromString(TEXT("Oxygen reserve depleted: supply disabled")), EShipAlertSeverity::Warning);
	}

	RebuildGlobalSnapshotFromModules();
}

void UShipSystemsComponent::RebuildGlobalSnapshotFromModules()
{
	if (Compartments.Num() == 0)
	{
		return;
	}

	float OxygenSum = 0.f;
	int32 TotalHotspots = 0;
	float MaxModuleFireIntensity = 0.f;
	for (const FShipCompartmentState& Module : Compartments)
	{
		OxygenSum += Module.OxygenLevel;
		TotalHotspots += Module.FireHotspotCount;
		MaxModuleFireIntensity = FMath::Max(MaxModuleFireIntensity, Module.FireIntensity);
	}

	OxygenLevel = OxygenSum / static_cast<float>(Compartments.Num());
	FireHotspotCount = TotalHotspots;
	FireIntensity = FMath::Clamp(MaxModuleFireIntensity, 0.f, 1.f);
}

bool UShipSystemsComponent::ValidateMandatoryModules()
{
	TArray<FText> Errors;
	const bool bIsValid = ValidateCurrentSetup(Errors);
	for (const FText& Error : Errors)
	{
		PushAlert(Error, EShipAlertSeverity::Critical);
	}
	LastConfigValidationErrors = Errors;
	return bIsValid;
}

bool UShipSystemsComponent::ValidateCurrentSetup(TArray<FText>& OutErrors) const
{
	OutErrors.Reset();

	const bool bHasReactor = Compartments.ContainsByPredicate([](const FShipCompartmentState& Module)
	{
		return Module.ModuleType == EShipModuleType::Reactor;
	});
	const bool bHasEngine = Compartments.ContainsByPredicate([](const FShipCompartmentState& Module)
	{
		return Module.ModuleType == EShipModuleType::Engine;
	});
	const bool bHasBridge = Compartments.ContainsByPredicate([](const FShipCompartmentState& Module)
	{
		return Module.ModuleType == EShipModuleType::Bridge;
	});
	const bool bHasAirlock = Compartments.ContainsByPredicate([](const FShipCompartmentState& Module)
	{
		return Module.ModuleType == EShipModuleType::Airlock;
	});
	const bool bHasOxygenStorageModule = Compartments.ContainsByPredicate([](const FShipCompartmentState& Module)
	{
		return Module.bProvidesOxygenStorage;
	});
	const bool bHasFuelStorageModule = Compartments.ContainsByPredicate([](const FShipCompartmentState& Module)
	{
		return Module.bProvidesFuelStorage;
	});

	if (!bHasReactor)
	{
		OutErrors.Add(FText::FromString(TEXT("Missing mandatory module type: Reactor")));
	}
	if (!bHasEngine)
	{
		OutErrors.Add(FText::FromString(TEXT("Missing mandatory module type: Engine")));
	}
	if (!bHasBridge)
	{
		OutErrors.Add(FText::FromString(TEXT("Missing mandatory module type: Bridge")));
	}
	if (!bHasAirlock)
	{
		OutErrors.Add(FText::FromString(TEXT("Missing mandatory module type: Airlock")));
	}
	if (!bHasOxygenStorageModule)
	{
		OutErrors.Add(FText::FromString(TEXT("Missing mandatory module capability: OxygenStorage")));
	}
	if (!bHasFuelStorageModule)
	{
		OutErrors.Add(FText::FromString(TEXT("Missing mandatory module capability: FuelStorage")));
	}

	if (OxygenReserve <= 0.f)
	{
		OutErrors.Add(FText::FromString(TEXT("Missing mandatory resource: OxygenReserve > 0")));
	}
	if (FuelReserve <= 0.f)
	{
		OutErrors.Add(FText::FromString(TEXT("Missing mandatory resource: FuelReserve > 0")));
	}

	const int32 Reactor = FindFirstModuleIndexByType(EShipModuleType::Reactor);
	const int32 Bridge = FindFirstModuleIndexByType(EShipModuleType::Bridge);
	if (Reactor != INDEX_NONE && Bridge != INDEX_NONE)
	{
		TArray<TArray<int32>> Graph;
		Graph.SetNum(Compartments.Num());
		for (const FShipBulkheadState& Bulkhead : Bulkheads)
		{
			const int32 A = FindModuleIndexById(Bulkhead.ModuleA);
			const int32 B = FindModuleIndexById(Bulkhead.ModuleB);
			if (A == INDEX_NONE || B == INDEX_NONE)
			{
				continue;
			}
			Graph[A].Add(B);
			Graph[B].Add(A);
		}

		TArray<bool> Visited;
		Visited.Init(false, Compartments.Num());
		TQueue<int32> Queue;
		Queue.Enqueue(Reactor);
		Visited[Reactor] = true;

		bool bConnected = false;
		while (!Queue.IsEmpty())
		{
			int32 Current = INDEX_NONE;
			Queue.Dequeue(Current);
			if (Current == Bridge)
			{
				bConnected = true;
				break;
			}

			for (int32 Next : Graph[Current])
			{
				if (!Visited[Next])
				{
					Visited[Next] = true;
					Queue.Enqueue(Next);
				}
			}
		}
		if (!bConnected)
		{
			OutErrors.Add(FText::FromString(TEXT("Invalid topology: Reactor and Bridge are disconnected")));
		}
	}

	return OutErrors.Num() == 0;
}

bool UShipSystemsComponent::ApplyShipConfig(const UShipConfigAsset* ConfigAsset)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !ConfigAsset)
	{
		return false;
	}

	TArray<FText> Errors;
	if (!ConfigAsset->ValidateConfig(Errors))
	{
		LastConfigValidationErrors = Errors;
		for (const FText& Error : Errors)
		{
			PushAlert(Error, EShipAlertSeverity::Critical);
		}
		return false;
	}

	Compartments.Reset();
	for (const FShipModuleConfig& ModuleConfig : ConfigAsset->Modules)
	{
		FShipCompartmentState ModuleState;
		const UShipModuleDefinition* Definition = ModuleConfig.ModuleDefinition.LoadSynchronous();
		ModuleState.ModuleId = ModuleConfig.ModuleId;
		if (ModuleState.ModuleId.IsNone() && Definition)
		{
			ModuleState.ModuleId = Definition->ModuleDefinitionId;
		}
		ModuleState.ModuleType = Definition ? Definition->ModuleType : ModuleConfig.ModuleType;
		ModuleState.bMandatory = ModuleConfig.bMandatory;
		ModuleState.bProvidesOxygenStorage = Definition && Definition->HasCapability(EShipModuleCapability::OxygenStorage);
		ModuleState.bProvidesFuelStorage = Definition && Definition->HasCapability(EShipModuleCapability::FuelStorage);
		ModuleState.OxygenLevel = ModuleConfig.OxygenLevel;
		ModuleState.FireHotspotCount = ModuleConfig.FireHotspotCount;
		ModuleState.FireIntensity = FMath::Clamp(static_cast<float>(ModuleState.FireHotspotCount) / FMath::Max(1, MaxFireHotspots), 0.f, 1.f);
		ModuleState.BreachSeverity = ModuleConfig.BreachSeverity;
		Compartments.Add(ModuleState);
	}

	const TArray<FShipBulkheadState> PreviousBulkheads = Bulkheads;
	Bulkheads.Reset();
	for (const FShipConnectionConfig& Connection : ConfigAsset->Connections)
	{
		FShipBulkheadState Bulkhead;
		Bulkhead.ModuleA = Connection.ModuleA;
		Bulkhead.ModuleB = Connection.ModuleB;
		Bulkhead.bOpen = Connection.bOpenByDefault;
		Bulkheads.Add(Bulkhead);
	}
	if (!AreBulkheadsEquivalent(PreviousBulkheads, Bulkheads))
	{
		OnBulkheadsChanged.Broadcast();
	}

	OxygenReserve = ConfigAsset->OxygenReserve;
	FuelReserve = ConfigAsset->FuelReserve;
	ActiveShipConfigId = ConfigAsset->ConfigId.IsNone() ? ConfigAsset->GetFName() : ConfigAsset->ConfigId;

	TArray<FText> LocalErrors;
	ValidateCurrentSetup(LocalErrors);
	LastConfigValidationErrors = LocalErrors;
	for (const FText& Error : LocalErrors)
	{
		PushAlert(Error, EShipAlertSeverity::Critical);
	}

	RebuildGlobalSnapshotFromModules();
	PushAlert(FText::Format(FText::FromString(TEXT("Applied ship config: {0}")), FText::FromName(ActiveShipConfigId)), EShipAlertSeverity::Info);
	return LocalErrors.Num() == 0;
}

int32 UShipSystemsComponent::FindModuleIndexById(FName ModuleId) const
{
	for (int32 i = 0; i < Compartments.Num(); ++i)
	{
		if (Compartments[i].ModuleId == ModuleId)
		{
			return i;
		}
	}
	return INDEX_NONE;
}

int32 UShipSystemsComponent::FindFirstModuleIndexByType(EShipModuleType ModuleType) const
{
	for (int32 i = 0; i < Compartments.Num(); ++i)
	{
		if (Compartments[i].ModuleType == ModuleType)
		{
			return i;
		}
	}
	return INDEX_NONE;
}

int32 UShipSystemsComponent::FindBulkheadIndex(FName ModuleA, FName ModuleB) const
{
	for (int32 i = 0; i < Bulkheads.Num(); ++i)
	{
		const FShipBulkheadState& Bulkhead = Bulkheads[i];
		const bool bSameDirection = Bulkhead.ModuleA == ModuleA && Bulkhead.ModuleB == ModuleB;
		const bool bOppositeDirection = Bulkhead.ModuleA == ModuleB && Bulkhead.ModuleB == ModuleA;
		if (bSameDirection || bOppositeDirection)
		{
			return i;
		}
	}
	return INDEX_NONE;
}

bool UShipSystemsComponent::AreBulkheadsEquivalent(const TArray<FShipBulkheadState>& A, const TArray<FShipBulkheadState>& B) const
{
	if (A.Num() != B.Num())
	{
		return false;
	}

	auto BuildNormalized = [](const TArray<FShipBulkheadState>& Source) -> TMap<FString, bool>
	{
		TMap<FString, bool> Result;
		for (const FShipBulkheadState& Item : Source)
		{
			const FString SA = Item.ModuleA.ToString();
			const FString SB = Item.ModuleB.ToString();
			const FString Key = (SA <= SB) ? (SA + TEXT("|") + SB) : (SB + TEXT("|") + SA);
			Result.Add(Key, Item.bOpen);
		}
		return Result;
	};

	const TMap<FString, bool> NA = BuildNormalized(A);
	const TMap<FString, bool> NB = BuildNormalized(B);
	if (NA.Num() != NB.Num())
	{
		return false;
	}

	for (const TPair<FString, bool>& Pair : NA)
	{
		const bool* State = NB.Find(Pair.Key);
		if (!State || *State != Pair.Value)
		{
			return false;
		}
	}
	return true;
}

bool UShipSystemsComponent::IsModuleConnectedToOxygenSource(int32 ModuleIndex) const
{
	if (!Compartments.IsValidIndex(ModuleIndex))
	{
		return false;
	}

	const int32 SourceIndex = FindModuleIndexById(OxygenSourceModuleId);
	if (SourceIndex == INDEX_NONE)
	{
		return true;
	}
	if (ModuleIndex == SourceIndex)
	{
		return true;
	}

	TArray<bool> Visited;
	Visited.Init(false, Compartments.Num());
	TQueue<int32> Queue;
	Queue.Enqueue(SourceIndex);
	Visited[SourceIndex] = true;

	while (!Queue.IsEmpty())
	{
		int32 Current = INDEX_NONE;
		Queue.Dequeue(Current);
		if (Current == ModuleIndex)
		{
			return true;
		}

		for (const FShipBulkheadState& Bulkhead : Bulkheads)
		{
			if (!Bulkhead.bOpen)
			{
				continue;
			}

			const int32 A = FindModuleIndexById(Bulkhead.ModuleA);
			const int32 B = FindModuleIndexById(Bulkhead.ModuleB);
			if (A == INDEX_NONE || B == INDEX_NONE)
			{
				continue;
			}

			int32 Next = INDEX_NONE;
			if (Current == A)
			{
				Next = B;
			}
			else if (Current == B)
			{
				Next = A;
			}

			if (Next != INDEX_NONE && !Visited[Next])
			{
				Visited[Next] = true;
				Queue.Enqueue(Next);
			}
		}
	}

	return false;
}

bool UShipSystemsComponent::SetBulkheadOpen(FName ModuleA, FName ModuleB, bool bOpen)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return false;
	}

	const int32 BulkheadIndex = FindBulkheadIndex(ModuleA, ModuleB);
	if (BulkheadIndex == INDEX_NONE)
	{
		return false;
	}

	FShipBulkheadState& Bulkhead = Bulkheads[BulkheadIndex];
	if (Bulkhead.bOpen == bOpen)
	{
		return true;
	}

	Bulkhead.bOpen = bOpen;
	OnBulkheadsChanged.Broadcast();
	PushAlert(
		FText::Format(
			FText::FromString(TEXT("Bulkhead {0}<->{1} {2}")),
			FText::FromName(Bulkhead.ModuleA),
			FText::FromName(Bulkhead.ModuleB),
			bOpen ? FText::FromString(TEXT("opened")) : FText::FromString(TEXT("closed"))
		),
		bOpen ? EShipAlertSeverity::Info : EShipAlertSeverity::Warning
	);
	return true;
}

bool UShipSystemsComponent::GetCompartmentStateById(FName ModuleId, FShipCompartmentState& OutState) const
{
	const int32 ModuleIndex = FindModuleIndexById(ModuleId);
	if (ModuleIndex == INDEX_NONE)
	{
		return false;
	}

	OutState = Compartments[ModuleIndex];
	return true;
}

bool UShipSystemsComponent::IsCompartmentConnectedToOxygenSource(FName ModuleId) const
{
	const int32 ModuleIndex = FindModuleIndexById(ModuleId);
	return IsModuleConnectedToOxygenSource(ModuleIndex);
}

bool UShipSystemsComponent::GetBulkheadOpenState(FName ModuleA, FName ModuleB, bool& bOutOpen) const
{
	const int32 BulkheadIndex = FindBulkheadIndex(ModuleA, ModuleB);
	if (BulkheadIndex == INDEX_NONE)
	{
		return false;
	}

	bOutOpen = Bulkheads[BulkheadIndex].bOpen;
	return true;
}

bool UShipSystemsComponent::AddFireHotspotToModule(FName ModuleId, int32 Count)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return false;
	}

	const int32 ModuleIndex = FindModuleIndexById(ModuleId);
	if (ModuleIndex == INDEX_NONE || Count <= 0)
	{
		return false;
	}

	FShipCompartmentState& Module = Compartments[ModuleIndex];
	Module.FireHotspotCount = FMath::Clamp(Module.FireHotspotCount + Count, 0, MaxFireHotspots);
	Module.FireIntensity = FMath::Clamp(static_cast<float>(Module.FireHotspotCount) / FMath::Max(1, MaxFireHotspots), 0.f, 1.f);
	RebuildGlobalSnapshotFromModules();
	return true;
}

bool UShipSystemsComponent::SetModuleBreachSeverity(FName ModuleId, float NewSeverity)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return false;
	}

	const int32 ModuleIndex = FindModuleIndexById(ModuleId);
	if (ModuleIndex == INDEX_NONE)
	{
		return false;
	}

	Compartments[ModuleIndex].BreachSeverity = FMath::Clamp(NewSeverity, 0.f, 1.f);
	return true;
}

void UShipSystemsComponent::PushAlert(const FText& Message, EShipAlertSeverity Severity)
{
	FShipAlertEntry Entry;
	Entry.Message = Message;
	Entry.Severity = Severity;
	Entry.ServerTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;

	RecentAlerts.Add(Entry);
	constexpr int32 MaxAlerts = 8;
	if (RecentAlerts.Num() > MaxAlerts)
	{
		RecentAlerts.RemoveAt(0, RecentAlerts.Num() - MaxAlerts);
	}
	OnShipAlert.Broadcast(Entry);
}

bool UShipSystemsComponent::ResolveCrewRole(AController* Issuer, UCrewRoleComponent*& OutRole)
{
	OutRole = nullptr;
	if (!Issuer)
	{
		return false;
	}
	APawn* Pawn = Issuer->GetPawn();
	if (!Pawn)
	{
		return false;
	}
	OutRole = Pawn->FindComponentByClass<UCrewRoleComponent>();
	return OutRole != nullptr;
}

bool UShipSystemsComponent::ResolveCrewRole(AController* Issuer, APawn* FallbackPawn, UCrewRoleComponent*& OutRole)
{
	OutRole = nullptr;
	if (Issuer)
	{
		if (APawn* Pawn = Issuer->GetPawn())
		{
			OutRole = Pawn->FindComponentByClass<UCrewRoleComponent>();
		}
	}
	if (!OutRole && FallbackPawn)
	{
		OutRole = FallbackPawn->FindComponentByClass<UCrewRoleComponent>();
	}
	return OutRole != nullptr;
}

bool UShipSystemsComponent::CanIssuerUseStation(AController* Issuer, FName StationPermission) const
{
	UCrewRoleComponent* Role = nullptr;
	if (!ResolveCrewRole(const_cast<AController*>(Issuer), Role) || !Role)
	{
		return false;
	}
	return Role->CanUseStation(StationPermission);
}

bool UShipSystemsComponent::ApplyAuthorizedAction(AController* Issuer, FName StationPermission, FName ActionId, float Magnitude, APawn* InstigatorPawn)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return false;
	}
	return InternalApplyAction(Issuer, StationPermission, ActionId, Magnitude, InstigatorPawn);
}

bool UShipSystemsComponent::InternalApplyAction(AController* Issuer, FName StationPermission, FName ActionId, float Magnitude, APawn* InstigatorPawn)
{
	UCrewRoleComponent* Role = nullptr;
	if (!ResolveCrewRole(Issuer, InstigatorPawn, Role) || !Role || !Role->CanUseStation(StationPermission))
	{
		return false;
	}

	// Дискретные действия для MVP — позже можно расширить через DataTable.
	if (ActionId == FName(TEXT("AdjustReactor")))
	{
		ReactorOutput = FMath::Clamp(ReactorOutput + 0.1f * Magnitude, 0.f, 1.2f);
		return true;
	}
	if (ActionId == FName(TEXT("IncreaseReactorPower")))
	{
		ReactorOutput = FMath::Clamp(ReactorOutput + 0.1f * FMath::Abs(Magnitude), 0.f, 1.2f);
		return true;
	}
	if (ActionId == FName(TEXT("DecreaseReactorPower")))
	{
		ReactorOutput = FMath::Clamp(ReactorOutput - 0.1f * FMath::Abs(Magnitude), 0.f, 1.2f);
		return true;
	}
	if (ActionId == FName(TEXT("VentAtmosphere")))
	{
		if (Compartments.Num() == 0)
		{
			InitializeDefaultModuleLayout();
		}
		const float Delta = 1.0f * FMath::Abs(Magnitude);
		for (FShipCompartmentState& Module : Compartments)
		{
			Module.OxygenLevel = FMath::Clamp(Module.OxygenLevel - Delta, 0.f, TargetOxygenPercent);
		}
		RebuildGlobalSnapshotFromModules();
		return true;
	}
	if (ActionId == FName(TEXT("ToggleOxygenSupply")))
	{
		bOxygenSupplyEnabled = (Magnitude >= 0.f);
		PushAlert(
			bOxygenSupplyEnabled
				? FText::FromString(TEXT("Oxygen supply enabled"))
				: FText::FromString(TEXT("Oxygen supply disabled")),
			bOxygenSupplyEnabled ? EShipAlertSeverity::Info : EShipAlertSeverity::Warning
		);
		return true;
	}
	if (ActionId == FName(TEXT("DisableOxygenSupply")))
	{
		bOxygenSupplyEnabled = false;
		PushAlert(FText::FromString(TEXT("Oxygen supply disabled")), EShipAlertSeverity::Warning);
		return true;
	}
	if (ActionId == FName(TEXT("EnableOxygenSupply")))
	{
		if (OxygenReserve > 0.f)
		{
			bOxygenSupplyEnabled = true;
			PushAlert(FText::FromString(TEXT("Oxygen supply enabled")), EShipAlertSeverity::Info);
		}
		else
		{
			PushAlert(FText::FromString(TEXT("Cannot enable oxygen supply: reserve is empty")), EShipAlertSeverity::Warning);
		}
		return true;
	}
	if (ActionId == FName(TEXT("RepairHull")))
	{
		HullIntegrity = FMath::Clamp(HullIntegrity + 0.1f * Magnitude, 0.f, 1.f);
		return true;
	}
	if (ActionId == FName(TEXT("FightFire")))
	{
		const int32 ExtinguishCount = FMath::Max(1, FMath::RoundToInt(FMath::Abs(Magnitude)));
		for (int32 i = 0; i < ExtinguishCount; ++i)
		{
			RemoveOneFireHotspot(InstigatorPawn);
		}
		return true;
	}
	if (ActionId == FName(TEXT("SealReactorEngineBulkhead")))
	{
		return SetBulkheadOpen(FName(TEXT("Reactor")), FName(TEXT("Engine")), false);
	}
	if (ActionId == FName(TEXT("OpenReactorEngineBulkhead")))
	{
		return SetBulkheadOpen(FName(TEXT("Reactor")), FName(TEXT("Engine")), true);
	}
	if (ActionId == FName(TEXT("SealEngineBridgeBulkhead")))
	{
		return SetBulkheadOpen(FName(TEXT("Engine")), FName(TEXT("Bridge")), false);
	}
	if (ActionId == FName(TEXT("OpenEngineBridgeBulkhead")))
	{
		return SetBulkheadOpen(FName(TEXT("Engine")), FName(TEXT("Bridge")), true);
	}
	if (ActionId == FName(TEXT("SetReactorBreach")))
	{
		return SetModuleBreachSeverity(FName(TEXT("Reactor")), FMath::Abs(Magnitude));
	}
	if (ActionId == FName(TEXT("SetEngineBreach")))
	{
		return SetModuleBreachSeverity(FName(TEXT("Engine")), FMath::Abs(Magnitude));
	}
	if (ActionId == FName(TEXT("SetBridgeBreach")))
	{
		return SetModuleBreachSeverity(FName(TEXT("Bridge")), FMath::Abs(Magnitude));
	}
	if (ActionId == FName(TEXT("StartFireEvent"))) // отладка / директор миссий
	{
		TriggerReactorFire(FText::FromString(TEXT("Debug fire event started")), EShipAlertSeverity::Warning);
		return true;
	}

	UE_LOG(LogSpaceshipCrew, Warning, TEXT("InternalApplyAction: неизвестный ActionId '%s' — добавьте ветку в ShipSystemsComponent или исправьте BP станции."),
		*ActionId.ToString());
	return false;
}

