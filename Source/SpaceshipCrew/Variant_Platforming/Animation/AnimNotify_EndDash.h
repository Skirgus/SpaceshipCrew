// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_EndDash.generated.h"

/**
 * AnimNotify для finish dash animation и restore игрок control
 */
UCLASS()
class UAnimNotify_EndDash : public UAnimNotify
{
	GENERATED_BODY()
	
public:

	/** Выполняет Anim Notify */
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	/** Возвращает имя нотифая */
	virtual FString GetNotifyName_Implementation() const override;
};




