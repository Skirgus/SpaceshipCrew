// Copyright Epic Games, Inc. All Rights Reserved.


#include "EnvQueryContext_Player.h"
#include "Kismet/GameplayStatics.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Actor.h"
#include "GameFramework/Pawn.h"

void UEnvQueryContext_Player::ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const
{
	// получить игрок pawn для первый локальный игрок
	AActor* PlayerPawn = UGameplayStatics::GetPlayerPawn(QueryInstance.Owner.Get(), 0);
	check(PlayerPawn);

	// add актор data для контекст
	UEnvQueryItemType_Actor::SetContextHelper(ContextData, PlayerPawn);
}




