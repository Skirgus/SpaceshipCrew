#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ShipBuilder/ShipBlueprintTypes.h"
#include "ShipBlueprintDefinition.generated.h"

/**
 * Проектный чертёж корабля (шаблон).
 * Хранение: Content/Data/Ships/ (/Game/Data/Ships), PrimaryAssetType "ShipBlueprint".
 */
UCLASS(BlueprintType)
class SPACESHIPCREW_API UShipBlueprintDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FName ShipId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Blueprint")
	FShipBlueprintDocument Document;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/** Собирает документ из полей ассета (синхронизирует ShipId/DisplayName). */
	FShipBlueprintDocument BuildDocument() const;
};
