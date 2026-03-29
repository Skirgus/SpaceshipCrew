// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ShipSystemsComponent.generated.h"

class UCrewRoleComponent;
class AController;
class APawn;

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

	/** Нормализованная мощность реактора (0-1). */
	UPROPERTY(ReplicatedUsing = OnRep_Snapshot, BlueprintReadOnly, Category = "Ship|Power")
	float ReactorOutput = 0.75f;

	/** Уровень кислорода (0-1), агрегировано по комнатам в рамках MVP. */
	UPROPERTY(ReplicatedUsing = OnRep_Snapshot, BlueprintReadOnly, Category = "Ship|Atmosphere")
	float OxygenLevel = 1.f;

	/** Целостность корпуса (0-1). */
	UPROPERTY(ReplicatedUsing = OnRep_Snapshot, BlueprintReadOnly, Category = "Ship|Damage")
	float HullIntegrity = 1.f;

	/** Интенсивность активного пожара (0-1). */
	UPROPERTY(ReplicatedUsing = OnRep_Snapshot, BlueprintReadOnly, Category = "Ship|Fire")
	float FireIntensity = 0.f;

	/**
	 * Проверяет роль инициатора относительно StationPermission, затем применяет ActionId (расширяемо).
	 * InstigatorPawn — запасной источник UCrewRoleComponent, если на сервере у пешки ещё нет Controller (редко при RPC).
	 */
	UFUNCTION(BlueprintCallable, Category = "Ship")
	bool ApplyAuthorizedAction(AController* Issuer, FName StationPermission, FName ActionId, float Magnitude = 1.f, APawn* InstigatorPawn = nullptr);

	UFUNCTION(BlueprintPure, Category = "Ship")
	bool CanIssuerUseStation(AController* Issuer, FName StationPermission) const;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_Snapshot();

	void SimulateSystems(float DeltaTime);

	bool InternalApplyAction(AController* Issuer, FName StationPermission, FName ActionId, float Magnitude, APawn* InstigatorPawn);

	static bool ResolveCrewRole(AController* Issuer, UCrewRoleComponent*& OutRole);
	static bool ResolveCrewRole(AController* Issuer, APawn* FallbackPawn, UCrewRoleComponent*& OutRole);
};

