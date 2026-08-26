#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameFramework/Actor.h"
#include "ShipModuleTypes.h"
#include "ShipModuleVisualOverride.generated.h"

/**
 * Один визуальный элемент кастомного представления модуля.
 * Transform задаётся в локальных координатах модуля (центр модуля = 0,0,0).
 *
 * Стены, привязанные к panel-socket (WallSocketName), при стыковке заменяются на
 * OpeningMesh / OpeningActorClass по OpeningKind — вместо глухой стены.
 */
USTRUCT(BlueprintType)
struct FShipModuleVisualPart
{
	GENERATED_BODY()

	/** Меш элемента (глухая стена / декор). Если не задан и нет Opening* — запись игнорируется. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TObjectPtr<UStaticMesh> Mesh = nullptr;

	/** Опциональный actor class для элемента (например, кресло/консоль как Blueprint Actor). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TSoftClassPtr<AActor> ActorClass;

	/** Локальный transform элемента относительно центра модуля. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	FTransform RelativeTransform = FTransform::Identity;

	/**
	 * Panel-socket, к которому привязана эта стена (например Front_X0_Y0_Z0).
	 * NAME_None — часть никогда не заменяется при стыковке (пол, декор, свет).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Docking|WallSlot")
	FName WallSocketName = NAME_None;

	/** Чем заменить стену, когда сокет открыт (стык с interior-соседом или ForcedOpening). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Docking|WallSlot")
	EShipModuleWallOpeningKind OpeningKind = EShipModuleWallOpeningKind::Passage;

	/**
	 * Меш открытого состояния (рамка проёма / открытая раздвижная дверь).
	 * Если пусто при открытом сокете — глухая стена не рисуется (пустой проём).
	 * OpeningMesh должен совпадать по локальным осям с RelativeTransform, либо
	 * задайте bUseOpeningRelativeTransform + OpeningRelativeTransform.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Docking|WallSlot")
	TObjectPtr<UStaticMesh> OpeningMesh = nullptr;

	/**
	 * Отдельный локальный transform для OpeningMesh (например FB-дверь с yaw 0/180
	 * при глухой стене LR с yaw ±90). Если false — используется RelativeTransform.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Docking|WallSlot")
	bool bUseOpeningRelativeTransform = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Docking|WallSlot", meta = (EditCondition = "bUseOpeningRelativeTransform"))
	FTransform OpeningRelativeTransform = FTransform::Identity;

	/**
	 * Actor раздвижной двери / интерактивного проёма (будущий runtime и PIE).
	 * В MVP-превью конструктора приоритет у OpeningMesh; ActorClass — для Module Editor и геймплея.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Docking|WallSlot")
	TSoftClassPtr<AActor> OpeningActorClass;

	/**
	 * Задел под life support: закрытая дверь герметична (не смешивает O2-объёмы).
	 * Имеет смысл при OpeningKind == SlidingDoor.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Docking|LifeSupport")
	bool bAirtightWhenClosed = true;

	/** Участвует ли этот проём в графе кислородных объёмов (будущий менеджмент O2). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Docking|LifeSupport")
	bool bAffectsOxygenVolume = true;
};

/**
 * Ручной визуальный override для ModuleDefinition.
 *
 * Если у модуля назначен этот ассет и в нём есть VisualParts, билдер использует
 * их вместо процедурной "коробки". Стены с WallSocketName меняются на проём/дверь
 * при стыковке. Также ассет может переопределять контактные точки.
 */
UCLASS(BlueprintType)
class SPACESHIPCREW_API UShipModuleVisualOverride : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Набор вручную отредактированных мешей для модуля. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TArray<FShipModuleVisualPart> VisualParts;

	/** Использовать контактные точки из этого ассета вместо ModuleDefinition.ContactPoints. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Docking")
	bool bOverrideContactPoints = false;

	/**
	 * Контактные точки override (при bOverrideContactPoints=true).
	 * Маркеры сокетов в редакторе модуля пишутся сюда — без записей в этом списке визуальных сокетов нет.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Docking", meta = (EditCondition = "bOverrideContactPoints"))
	TArray<FShipModuleContactPoint> ContactPointsOverride;
};
