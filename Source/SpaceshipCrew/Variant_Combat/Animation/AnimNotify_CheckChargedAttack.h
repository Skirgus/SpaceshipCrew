// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_CheckChargedAttack.generated.h"

/**
 * AnimNotify для perform a charged атака hold проверить.
 */
UCLASS()
class UAnimNotify_CheckChargedAttack : public UAnimNotify
{
	GENERATED_BODY()
	
public:

	/** Выполняет Anim Notify */
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	/** Возвращает имя нотифая */
	virtual FString GetNotifyName_Implementation() const override;
};




