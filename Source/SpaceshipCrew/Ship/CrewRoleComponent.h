// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CrewRoleComponent.generated.h"

class UCrewRoleDefinition;

/** Attached to crew pawns; replicated so server-authoritative station checks work. */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class SPACESHIPCREW_API UCrewRoleComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCrewRoleComponent();

	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_Role, BlueprintReadOnly, Category = "Crew")
	TObjectPtr<UCrewRoleDefinition> RoleDefinition;

	UFUNCTION(BlueprintPure, Category = "Crew")
	UCrewRoleDefinition* GetRoleDefinition() const { return RoleDefinition; }

	UFUNCTION(BlueprintPure, Category = "Crew")
	bool CanUseStation(FName StationPermission) const;

	void SetRoleDefinition(UCrewRoleDefinition* InRole);

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_Role();
};
