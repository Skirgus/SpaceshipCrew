// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryContext.h"
#include "EnvQueryContext_Player.generated.h"

/**
 * UEnvQueryContext_Игрок
 * Basic EnvQuery контекст который вернутьs первый локальный игрок
 */
UCLASS()
class UEnvQueryContext_Player : public UEnvQueryContext
{
	GENERATED_BODY()
	
public:

	/** Предоставляет контекстные позиции или акторов для этого EnvQuery */
	virtual void ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const override;
};




