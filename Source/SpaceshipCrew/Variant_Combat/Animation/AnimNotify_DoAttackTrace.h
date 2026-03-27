// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_DoAttackTrace.generated.h"

/**
 * AnimNotify для tell актор для perform атака trace проверить для look для targets для урон.
 */
UCLASS()
class UAnimNotify_DoAttackTrace : public UAnimNotify
{
	GENERATED_BODY()
	
protected:

	/** Source bone для атака trace */
	UPROPERTY(EditAnywhere, Category="Attack")
	FName AttackBoneName;

public:

	/** Выполняет Anim Notify */
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	/** Возвращает имя нотифая */
	virtual FString GetNotifyName_Implementation() const override;
};




