// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShipSystemsComponent.h"
#include "SpaceshipCrew.h"
#include "CrewRoleComponent.h"
#include "ShipInteractableBase.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"

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
	DOREPLIFETIME(UShipSystemsComponent, bOxygenSupplyEnabled);
	DOREPLIFETIME(UShipSystemsComponent, HullIntegrity);
	DOREPLIFETIME(UShipSystemsComponent, FireIntensity);
	DOREPLIFETIME(UShipSystemsComponent, FireHotspotCount);
	DOREPLIFETIME(UShipSystemsComponent, RecentAlerts);
}

void UShipSystemsComponent::OnRep_Snapshot()
{
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

void UShipSystemsComponent::SimulateSystems(float DeltaTime)
{
	EvaluateReactorFireRisk(DeltaTime);

	// Атмосфера: пожар выжигает O2; при включенной подаче система пытается держать 21% (если есть запас).
	if (FireIntensity > 0.01f)
	{
		const float FireOxygenBurnRate = 3.0f; // % O2 в секунду при FireIntensity=1
		OxygenLevel = FMath::Clamp(OxygenLevel - FireOxygenBurnRate * FireIntensity * DeltaTime, 0.f, TargetOxygenPercent);
	}

	if (bOxygenSupplyEnabled && OxygenReserve > 0.f && OxygenLevel < TargetOxygenPercent)
	{
		const float RefillRate = 4.0f; // % O2 в секунду
		const float Need = TargetOxygenPercent - OxygenLevel;
		const float Add = FMath::Min(Need, RefillRate * DeltaTime);
		OxygenLevel = FMath::Clamp(OxygenLevel + Add, 0.f, TargetOxygenPercent);

		// 1% восстановления атмосферы расходует 1 единицу запаса.
		OxygenReserve = FMath::Max(0.f, OxygenReserve - Add);
		if (OxygenReserve <= 0.01f)
		{
			bOxygenSupplyEnabled = false;
			PushAlert(FText::FromString(TEXT("Oxygen reserve depleted: supply disabled")), EShipAlertSeverity::Warning);
		}
	}

	if (OxygenLevel < 8.f)
	{
		HullIntegrity = FMath::Clamp(HullIntegrity - 0.01f * DeltaTime, 0.f, 1.f);
	}

	// При низком кислороде существующие очаги должны постепенно тухнуть.
	if (OxygenLevel <= MinOxygenPercentForNewFires && FireHotspotCount > 0)
	{
		// Чем ниже O2, тем быстрее затухание. Скорость настраивается через UPROPERTY в компоненте.
		const float SecondsPerHotspot = FMath::GetMappedRangeValueClamped(
			FVector2D(MinOxygenPercentForNewFires, 0.f),
			FVector2D(LowOxygenExtinguishSecondsAtThreshold, LowOxygenExtinguishSecondsAtZero),
			OxygenLevel
		);
		LowOxygenExtinguishProgress += DeltaTime / FMath::Max(0.2f, SecondsPerHotspot);
		while (LowOxygenExtinguishProgress >= 1.f && FireHotspotCount > 0)
		{
			LowOxygenExtinguishProgress -= 1.f;
			RemoveOneFireHotspot(nullptr);
			PushAlert(FText::FromString(TEXT("Fire hotspot extinguished by low oxygen")), EShipAlertSeverity::Info);
		}
	}
	else
	{
		LowOxygenExtinguishProgress = 0.f;
	}

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
}

void UShipSystemsComponent::EvaluateReactorFireRisk(float DeltaTime)
{
	const float Output = ReactorOutput;
	const bool bOverloaded = (Output > 1.0f);

	// При низком O2 новые очаги пожара не возникают.
	if (OxygenLevel <= MinOxygenPercentForNewFires)
	{
		OverloadGuaranteeProgress = 0.f;
		bLastOverloadState = false;
		if (!bLowOxygenFireSuppressionAlerted)
		{
			PushAlert(
				FText::Format(
					FText::FromString(TEXT("Oxygen {0}% <= {1}%: new fire hotspots are suppressed")),
					FText::AsNumber(FMath::RoundToInt(OxygenLevel)),
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
	if (FireHotspotCount < MaxFireHotspots)
	{
		++FireHotspotCount;
		SyncFireIntensityFromHotspots();
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
	--FireHotspotCount;
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
	FireIntensity = FMath::Clamp(static_cast<float>(FireHotspotCount) / FMath::Max(1, MaxFireHotspots), 0.f, 1.f);
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
		OxygenLevel = FMath::Clamp(OxygenLevel - 1.0f * Magnitude, 0.f, TargetOxygenPercent);
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
	if (ActionId == FName(TEXT("StartFireEvent"))) // отладка / директор миссий
	{
		TriggerReactorFire(FText::FromString(TEXT("Debug fire event started")), EShipAlertSeverity::Warning);
		return true;
	}

	UE_LOG(LogSpaceshipCrew, Warning, TEXT("InternalApplyAction: неизвестный ActionId '%s' — добавьте ветку в ShipSystemsComponent или исправьте BP станции."),
		*ActionId.ToString());
	return false;
}

