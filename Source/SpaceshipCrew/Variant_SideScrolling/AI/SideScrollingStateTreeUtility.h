// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"

#include "SideScrollingStateTreeUtility.generated.h"

class AAIController;

/**
 * Instance data для FStateTreeGetИгрокTask task
 */
USTRUCT()
struct FStateTreeGetPlayerInstanceData
{
	GENERATED_BODY()

	/** NPC owning этот task */
	UPROPERTY(VisibleAnywhere, Category="Context")
	TObjectPtr<APawn> NPC;

	/** Holds found игрок pawn */
	UPROPERTY(VisibleAnywhere, Category="Context")
	TObjectPtr<AAIController> Controller;

	/** Holds found игрок pawn */
	UPROPERTY(VisibleAnywhere, Category="Output")
	TObjectPtr<APawn> TargetPlayer;

	/** Is pawn close enough для be considered a валидный цель? */
	UPROPERTY(VisibleAnywhere, Category="Output")
	bool bValidTarget = false;

	/** Max дистанция для be considered a валидный цель */
	UPROPERTY(EditAnywhere, Category="Parameter", meta = (ClampMin = 0, ClampMax = 10000, Units = "cm"))
	float RangeMax = 1000.0f;
};

/**
 * StateTree task для получить игрок-controlled персонаж
 */
USTRUCT(meta=(DisplayName="Get Player", Category="Side Scrolling"))
struct FStateTreeGetPlayerTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	/* Ensure we're using the correct instance data struct */
	using FInstanceDataType = FStateTreeGetPlayerInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	/** Runs пока owning состояние is active */
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif // WITH_EDITOR
};



