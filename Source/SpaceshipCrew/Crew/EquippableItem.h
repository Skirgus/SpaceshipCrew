#pragma once

#include "CoreMinimal.h"
#include "CrewInteractable.h"
#include "GameFramework/Actor.h"
#include "EquippableItem.generated.h"

class APawn;
class UAnimMontage;
class UBoxComponent;
class UPrimitiveComponent;
class USceneComponent;
class USkeletalMeshComponent;
class UStaticMeshComponent;
class UTexture2D;
class UUserWidget;

/**
 * База экипируемого предмета (отвертка, горелка, оружие).
 * Меш обязателен как слот. UI, анимации и secondary — необязательны.
 * Конкретные типы — Blueprint-наследники. Не связан с AUsableEquipment.
 */
UCLASS(Abstract, Blueprintable)
class SPACESHIPCREW_API AEquippableItem : public AActor, public ICrewInteractable
{
	GENERATED_BODY()

public:
	AEquippableItem();

	virtual void BeginPlay() override;

	virtual bool CanInteract_Implementation(APawn* InstigatorPawn) const override;
	virtual void Interact_Implementation(APawn* InstigatorPawn) override;
	virtual FText GetInteractPrompt_Implementation(APawn* InstigatorPawn) const override;
	virtual FText GetInteractDisplayName_Implementation(APawn* InstigatorPawn) const override;
	virtual FVector GetPromptAnchorWorldLocation_Implementation() const override;
	virtual UPrimitiveComponent* GetInteractionPrimitive_Implementation() const override;

	UFUNCTION(BlueprintPure, Category = "Item")
	UBoxComponent* GetInteractionVolume() const { return InteractionVolume; }

	UFUNCTION(BlueprintPure, Category = "Item")
	UTexture2D* GetInventoryIcon() const { return InventoryIcon; }

	UFUNCTION(BlueprintPure, Category = "Item")
	FText GetItemDisplayName() const { return DisplayName; }

	UFUNCTION(BlueprintPure, Category = "Item")
	bool SupportsSecondaryAction() const { return bSupportsSecondaryAction; }

	/** Мир / в инвентаре / в руках. */
	UFUNCTION(BlueprintPure, Category = "Item")
	bool IsWorldPickup() const { return bIsWorldPickup; }

	UFUNCTION(BlueprintCallable, Category = "Item")
	void ConfigureAsHeldVisual();

	/** Вернуть предмет в состояние пикапа в мире. */
	UFUNCTION(BlueprintCallable, Category = "Item")
	void ConfigureAsWorldPickup();

	UFUNCTION(BlueprintCallable, Category = "Item")
	void AttachToCharacterHand(APawn* OwnerPawn);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Item")
	void OnPickedUp(APawn* Picker);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Item")
	void OnDropped(APawn* Dropper);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Item")
	void OnEquipped(APawn* User);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Item")
	void OnUnequipped(APawn* User);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Item")
	bool CanPrimaryUse(APawn* User) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Item")
	void OnPrimaryUse(APawn* User);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Item")
	void OnSecondaryUse(APawn* User);

	/** Имя на белой плашке подсказки. Пусто — на плашке только действие. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FText DisplayName;

	/** Действие справа от плашки («Взять»). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FText PromptText;

	/** Иконка для инвентаря и хотбара. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	TObjectPtr<UTexture2D> InventoryIcon;

	/** Пусто — без UI в руках (детектор / индикатор — задать класс). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|UI")
	TSubclassOf<UUserWidget> HeldWidgetClass;

	/** Монтаж на персонаже при экипировке в руки. Пусто — не играется. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Animation")
	TObjectPtr<UAnimMontage> CharacterEquipMontage;

	/** Монтаж на персонаже при PrimaryUse. Пусто — не играется. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Animation")
	TObjectPtr<UAnimMontage> CharacterUseMontage;

	/** Сокет скелета персонажа для аттача в руках. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FName HandSocketName = FName(TEXT("hand_r"));

	/** Если false — ПКМ ничего не делает. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	bool bSupportsSecondaryAction = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FVector HeldRelativeLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FRotator HeldRelativeRotation = FRotator::ZeroRotator;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item")
	TObjectPtr<UStaticMeshComponent> DisplayMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item")
	TObjectPtr<USkeletalMeshComponent> AnimatedMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item")
	TObjectPtr<UBoxComponent> InteractionVolume;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item")
	TObjectPtr<USceneComponent> PromptAnchor;

	void ShowHeldUI(APawn* User);
	void HideHeldUI();
	void PlayCharacterMontage(APawn* User, UAnimMontage* Montage) const;
	void EnableWorldPhysics();
	void DisableWorldPhysics();

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> ActiveHeldWidget;

	/** true пока лежит в мире как пикап; false после подбора / для held-визуала. */
	UPROPERTY(Transient)
	bool bIsWorldPickup = true;
};
