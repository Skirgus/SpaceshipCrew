// Copyright Epic Games, Inc. All Rights Reserved.


#include "AnimNotify_CheckCombo.h"
#include "CombatAttacker.h"
#include "Components/SkeletalMeshComponent.h"

void UAnimNotify_CheckCombo::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	// cast owner для атакаer интерфейс
	if (ICombatAttacker* AttackerInterface = Cast<ICombatAttacker>(MeshComp->GetOwner()))
	{
 // tell актор для проверить для combo string
		AttackerInterface->CheckCombo();
	}
}

FString UAnimNotify_CheckCombo::GetNotifyName_Implementation() const
{
	return FString("Check Combo String");
}




