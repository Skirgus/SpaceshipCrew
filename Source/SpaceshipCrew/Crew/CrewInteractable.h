#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CrewInteractable.generated.h"

class APawn;

UINTERFACE(MinimalAPI, BlueprintType)
class UCrewInteractable : public UInterface
{
	GENERATED_BODY()
};

/**
 * Контракт взаимодействия экипажа (клавиша Interact / E).
 * Реализуют рабочие станции, панели ремонта, пикапы.
 */
class SPACESHIPCREW_API ICrewInteractable
{
	GENERATED_BODY()

public:
	/** Можно ли начать взаимодействие этим пешком. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Crew|Interact")
	bool CanInteract(APawn* InstigatorPawn) const;

	/** Выполнить взаимодействие (occupy, pickup, начать hold-repair и т.п.). */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Crew|Interact")
	void Interact(APawn* InstigatorPawn);

	/** Подсказка для HUD (краткий текст). */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Crew|Interact")
	FText GetInteractPrompt(APawn* InstigatorPawn) const;
};
