#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SpaceshipCrew.h"
#include "SpaceshipCrewMenuWidgetBase.generated.h"

/**
 * Базовый UMG UserWidget для экранов меню и вложенных панелей.
 * Дальнейшая визуальная сборка — в Blueprint-наследниках; общая логика и первичная разметка могут задаваться в NativeConstruct в C++.
 * Параллельно для низкоуровневого Slate используйте SSpaceshipMainMenuBase и его наследников.
 */
UCLASS(Abstract, Blueprintable, BlueprintType)
class SPACESHIPCREW_API USpaceshipCrewMenuWidgetBase : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
};
