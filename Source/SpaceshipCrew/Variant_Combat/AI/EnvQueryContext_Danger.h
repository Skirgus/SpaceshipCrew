// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryContext.h"
#include "EnvQueryContext_Danger.generated.h"

/**
 * UEnvQueryContext_Danger
 * Возвращает враг персонаж's последний known опасность location
 */
UCLASS()
class SPACESHIPCREW_API UEnvQueryContext_Danger : public UEnvQueryContext
{
	GENERATED_BODY()
	
public:

	/** Предоставляет контекстные позиции или акторов для этого EnvQuery */
	virtual void ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const override;

};




