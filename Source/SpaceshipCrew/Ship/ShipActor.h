// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShipActor.generated.h"

class UShipSystemsComponent;
class UStaticMeshComponent;
class UShipConfigAsset;
class UShipModuleDefinition;
class AShipDoorActor;
class UPointLightComponent;

/** Размещается на уровне; содержит авторитетные системы корабля. */
UCLASS(Blueprintable)
class SPACESHIPCREW_API AShipActor : public AActor
{
	GENERATED_BODY()

public:
	AShipActor();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Визуал и корень трансформации корабля; меш задаётся в BP или на инстансе. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ship|Visual")
	TObjectPtr<UStaticMeshComponent> HullMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ship")
	TObjectPtr<UShipSystemsComponent> ShipSystems;

	/** Фолбэк-конфиг, если GameMode не выбрал конфиг уровня. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Config")
	TSoftObjectPtr<UShipConfigAsset> DefaultShipConfig;

	/** Размер ячейки сетки модулей для MVP-визуализации. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Visual")
	float ModuleGridSize = 400.f;

	/** Центрировать модульную сетку вокруг ShipActor. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Visual")
	bool bCenterModuleGridAroundActor = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Visual")
	float InteriorLightIntensity = 1800.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Visual")
	float InteriorLightRadius = 650.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Visual")
	TSubclassOf<AShipDoorActor> DoorActorClass;

	UFUNCTION(BlueprintPure, Category = "Ship")
	UShipSystemsComponent* GetShipSystems() const { return ShipSystems; }

	UFUNCTION(BlueprintCallable, Category = "Ship|Config")
	bool ApplyShipConfigAsset(UShipConfigAsset* ShipConfigAsset);

protected:
	void RebuildModuleVisualsFromConfig(const UShipConfigAsset* ShipConfigAsset);
	void SyncRuntimeDoorsFromBulkheads();
	void HandleBulkheadsChanged();
	static FString MakeBulkheadKey(FName ModuleA, FName ModuleB);

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> RuntimeModuleMeshes;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UPointLightComponent>> RuntimeModuleLights;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AShipDoorActor>> RuntimeDoorActors;

	TMap<FString, TArray<TObjectPtr<AShipDoorActor>>> RuntimeDoorsByBulkheadKey;
};

