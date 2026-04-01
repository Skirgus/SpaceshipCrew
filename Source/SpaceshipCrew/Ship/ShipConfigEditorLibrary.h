// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ShipConfigAsset.h"
#include "ShipConfigEditorLibrary.generated.h"

UCLASS()
class SPACESHIPCREW_API UShipConfigEditorLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Ship|Config|Editor")
	static bool AddOrUpdateModule(UShipConfigAsset* Config, const FShipModuleConfig& Module);

	UFUNCTION(BlueprintCallable, Category = "Ship|Config|Editor")
	static bool RemoveModule(UShipConfigAsset* Config, FName ModuleId);

	UFUNCTION(BlueprintCallable, Category = "Ship|Config|Editor")
	static bool AddConnection(UShipConfigAsset* Config, const FShipConnectionConfig& Connection);

	UFUNCTION(BlueprintCallable, Category = "Ship|Config|Editor")
	static bool SetConnectionOpenByDefault(UShipConfigAsset* Config, FName ModuleA, FName ModuleB, bool bOpenByDefault);

	UFUNCTION(BlueprintCallable, Category = "Ship|Config|Editor")
	static bool SaveConfigAsset(UShipConfigAsset* Config);
};

