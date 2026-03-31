// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ShipSystemsComponent.generated.h"

class UCrewRoleComponent;
class AController;
class APawn;
class AShipInteractableBase;

UENUM(BlueprintType)
enum class EShipAlertSeverity : uint8
{
	Info UMETA(DisplayName = "Info"),
	Warning UMETA(DisplayName = "Warning"),
	Critical UMETA(DisplayName = "Critical")
};

USTRUCT(BlueprintType)
struct SPACESHIPCREW_API FShipAlertEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Ship|Alerts")
	FText Message;

	UPROPERTY(BlueprintReadOnly, Category = "Ship|Alerts")
	EShipAlertSeverity Severity = EShipAlertSeverity::Info;

	UPROPERTY(BlueprintReadOnly, Category = "Ship|Alerts")
	float ServerTimeSeconds = 0.f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShipAlertSignature, const FShipAlertEntry&, Alert);

/**
 * Сервер-авторитетная симуляция корабля. Команды проходят авторизацию по роли,
 * без разделения логики между игроком и ботом.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class SPACESHIPCREW_API UShipSystemsComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UShipSystemsComponent();

	/** Нагрузка реактора (0-1.2): 1.0 = 100%, 1.2 = 120% (аварийный режим). */
	UPROPERTY(ReplicatedUsing = OnRep_Snapshot, BlueprintReadOnly, Category = "Ship|Power")
	float ReactorOutput = 0.75f;

	/** Процент кислорода в атмосфере корабля (обычно поддерживается около 21%). */
	UPROPERTY(ReplicatedUsing = OnRep_Snapshot, BlueprintReadOnly, Category = "Ship|Atmosphere")
	float OxygenLevel = 21.f;

	/** Запас кислорода в системах корабля (условные единицы). */
	UPROPERTY(ReplicatedUsing = OnRep_Snapshot, BlueprintReadOnly, Category = "Ship|Atmosphere")
	float OxygenReserve = 100.f;

	/** Включена ли подача кислорода в атмосферу корабля. */
	UPROPERTY(ReplicatedUsing = OnRep_Snapshot, BlueprintReadOnly, Category = "Ship|Atmosphere")
	bool bOxygenSupplyEnabled = true;

	/** Целостность корпуса (0-1). */
	UPROPERTY(ReplicatedUsing = OnRep_Snapshot, BlueprintReadOnly, Category = "Ship|Damage")
	float HullIntegrity = 1.f;

	/** Интенсивность активного пожара (0-1). */
	UPROPERTY(ReplicatedUsing = OnRep_Snapshot, BlueprintReadOnly, Category = "Ship|Fire")
	float FireIntensity = 0.f;

	/** Последние тревоги корабля (реплицируются в HUD). */
	UPROPERTY(ReplicatedUsing = OnRep_RecentAlerts, BlueprintReadOnly, Category = "Ship|Alerts")
	TArray<FShipAlertEntry> RecentAlerts;

	/** Событие тревоги (срабатывает на сервере и клиентах через репликацию). */
	UPROPERTY(BlueprintAssignable, Category = "Ship|Alerts")
	FOnShipAlertSignature OnShipAlert;

	/** Класс станции тушения, которая спавнится рядом с реактором при пожаре. */
	UPROPERTY(EditAnywhere, Category = "Ship|Fire")
	TSubclassOf<AShipInteractableBase> FireStationClass;

	/** Смещение реактора относительно корабля (центр для спавна fire-станции). */
	UPROPERTY(EditAnywhere, Category = "Ship|Fire")
	FVector ReactorWorldOffset = FVector(250.f, 0.f, 60.f);

	/** Радиус случайного спавна fire-станции около реактора. */
	UPROPERTY(EditAnywhere, Category = "Ship|Fire", meta = (ClampMin = "50.0", ClampMax = "600.0"))
	float FireStationSpawnRadius = 180.f;

	/** Целевой процент кислорода при нормальной работе систем. */
	UPROPERTY(EditAnywhere, Category = "Ship|Atmosphere", meta = (ClampMin = "10.0", ClampMax = "30.0"))
	float TargetOxygenPercent = 21.f;

	/** При таком уровне O2 и ниже новые очаги пожара больше не возникают. */
	UPROPERTY(EditAnywhere, Category = "Ship|Fire", meta = (ClampMin = "1.0", ClampMax = "21.0"))
	float MinOxygenPercentForNewFires = 12.f;

	/** Время (сек) на затухание одного очага при O2 около порога MinOxygenPercentForNewFires. */
	UPROPERTY(EditAnywhere, Category = "Ship|Fire", meta = (ClampMin = "0.2", ClampMax = "20.0"))
	float LowOxygenExtinguishSecondsAtThreshold = 4.0f;

	/** Время (сек) на затухание одного очага при очень низком O2 (около 0%). */
	UPROPERTY(EditAnywhere, Category = "Ship|Fire", meta = (ClampMin = "0.2", ClampMax = "20.0"))
	float LowOxygenExtinguishSecondsAtZero = 1.2f;

	/** Максимальное число одновременно существующих очагов пожара. */
	UPROPERTY(EditAnywhere, Category = "Ship|Fire", meta = (ClampMin = "1", ClampMax = "12"))
	int32 MaxFireHotspots = 5;

	/** Текущее число очагов пожара (репликация в HUD). */
	UPROPERTY(ReplicatedUsing = OnRep_Snapshot, BlueprintReadOnly, Category = "Ship|Fire")
	int32 FireHotspotCount = 0;

	/**
	 * Проверяет роль инициатора относительно StationPermission, затем применяет ActionId (расширяемо).
	 * InstigatorPawn — запасной источник UCrewRoleComponent, если на сервере у пешки ещё нет Controller (редко при RPC).
	 */
	UFUNCTION(BlueprintCallable, Category = "Ship")
	bool ApplyAuthorizedAction(AController* Issuer, FName StationPermission, FName ActionId, float Magnitude = 1.f, APawn* InstigatorPawn = nullptr);

	UFUNCTION(BlueprintPure, Category = "Ship")
	bool CanIssuerUseStation(AController* Issuer, FName StationPermission) const;

	UFUNCTION(BlueprintPure, Category = "Ship|Alerts")
	TArray<FShipAlertEntry> GetRecentAlerts() const { return RecentAlerts; }

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_Snapshot();

	UFUNCTION()
	void OnRep_RecentAlerts();

	void SimulateSystems(float DeltaTime);
	void EvaluateReactorFireRisk(float DeltaTime);
	void TriggerReactorFire(const FText& Reason, EShipAlertSeverity Severity);
	void SpawnOneFireStation();
	void RemoveOneFireHotspot(APawn* InstigatorPawn);
	void SyncFireIntensityFromHotspots();
	void PushAlert(const FText& Message, EShipAlertSeverity Severity);

	bool InternalApplyAction(AController* Issuer, FName StationPermission, FName ActionId, float Magnitude, APawn* InstigatorPawn);

	static bool ResolveCrewRole(AController* Issuer, UCrewRoleComponent*& OutRole);
	static bool ResolveCrewRole(AController* Issuer, APawn* FallbackPawn, UCrewRoleComponent*& OutRole);

	/** Накопитель гарантированного пожара при перегрузе >100%. */
	float OverloadGuaranteeProgress = 0.f;

	/** Накопитель постепенного затухания очагов при низком уровне O2. */
	float LowOxygenExtinguishProgress = 0.f;

	/** Станции тушения по числу очагов пожара. */
	UPROPERTY()
	TArray<TObjectPtr<AShipInteractableBase>> ActiveFireStations;

	bool bLastOverloadState = false;
	bool bLowOxygenFireSuppressionAlerted = false;
};

