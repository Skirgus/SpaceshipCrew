#pragma once

#include "CoreMinimal.h"
#include "CrewInteractable.h"
#include "GameFramework/Actor.h"
#include "UsableEquipment.generated.h"

class APawn;
class UAnimMontage;
class UBoxComponent;
class UPrimitiveComponent;
class USceneComponent;
class USkeletalMeshComponent;
class UStaticMeshComponent;
class UUserWidget;

UENUM(BlueprintType)
enum class EUsableEquipmentUseMode : uint8
{
	/** Анимации и необязательный UI, персонаж остаётся на ногах. */
	Interact UMETA(DisplayName = "Interact"),
	/** Встать в точку использования, ходьба выключена. */
	Occupy UMETA(DisplayName = "Occupy"),
	/** Занять и отдать WASD оборудованию вместо шага. */
	OccupyRedirectInput UMETA(DisplayName = "Occupy Redirect Input")
};

/**
 * База используемого оборудования внутри модуля.
 * Меш обязателен как слот. UI и анимации необязательны.
 * Конкретные кресла, пульты и шкафчики — Blueprint-наследники.
 */
UCLASS(Abstract, Blueprintable)
class SPACESHIPCREW_API AUsableEquipment : public AActor, public ICrewInteractable
{
	GENERATED_BODY()

public:
	AUsableEquipment();

	virtual void BeginPlay() override;

	virtual bool CanInteract_Implementation(APawn* InstigatorPawn) const override;
	virtual void Interact_Implementation(APawn* InstigatorPawn) override;
	virtual FText GetInteractPrompt_Implementation(APawn* InstigatorPawn) const override;
	virtual FText GetInteractDisplayName_Implementation(APawn* InstigatorPawn) const override;
	virtual FVector GetPromptAnchorWorldLocation_Implementation() const override;
	virtual UPrimitiveComponent* GetInteractionPrimitive_Implementation() const override;

	UFUNCTION(BlueprintCallable, Category = "Equipment")
	bool BeginUse(APawn* User);

	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void EndUse();

	UFUNCTION(BlueprintPure, Category = "Equipment")
	bool IsInUse() const { return UserPawn != nullptr; }

	UFUNCTION(BlueprintPure, Category = "Equipment")
	bool IsInUseBy(const APawn* Pawn) const { return UserPawn != nullptr && UserPawn == Pawn; }

	UFUNCTION(BlueprintPure, Category = "Equipment")
	APawn* GetUser() const { return UserPawn; }

	UFUNCTION(BlueprintPure, Category = "Equipment")
	EUsableEquipmentUseMode GetUseMode() const { return UseMode; }

	/** Occupy и OccupyRedirectInput глушат шаг персонажа. */
	UFUNCTION(BlueprintPure, Category = "Equipment")
	bool IsMovementLocked() const;

	/** WASD уходит в OnRedirectedMove, а не в CharacterMovement. */
	UFUNCTION(BlueprintPure, Category = "Equipment")
	bool WantsMovementRedirect() const;

	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void NotifyRedirectedMove(FVector2D MoveInput);

	UFUNCTION(BlueprintPure, Category = "Equipment")
	UBoxComponent* GetInteractionVolume() const { return InteractionVolume; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	EUsableEquipmentUseMode UseMode = EUsableEquipmentUseMode::Interact;

	/** Имя на белой плашке подсказки. Пусто — на плашке только действие. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	FText DisplayName;

	/** Действие справа от плашки («Сесть», «Использовать»). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	FText PromptText;

	/** Пусто — без интерфейса (кресло «просто сесть»). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|UI")
	TSubclassOf<UUserWidget> InteractionWidgetClass;

	/** Монтаж на скелетном меше предмета. Пусто или без скелетного меша — не играется. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Animation")
	TObjectPtr<UAnimMontage> EquipmentUseMontage;

	/** Монтаж на персонаже. Пусто — не играется. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Animation")
	TObjectPtr<UAnimMontage> CharacterUseMontage;

	/**
	 * Куда сдвинуть персонажа относительно UseAnchor, когда он отпускает предмет.
	 * Для кресла это шаг вперёд: капсула выходит из сиденья, пока монтаж сходит в стойку.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	FVector ReleaseOffset = FVector::ZeroVector;

	UFUNCTION(BlueprintImplementableEvent, Category = "Equipment")
	void OnEquipmentUsed(APawn* User);

	UFUNCTION(BlueprintImplementableEvent, Category = "Equipment")
	void OnEquipmentReleased(APawn* User);

	/** Вызывается каждый кадр, пока режим OccupyRedirectInput и пользователь держит ввод. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Equipment")
	void OnRedirectedMove(FVector2D MoveInput);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment")
	TObjectPtr<UStaticMeshComponent> DisplayMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment")
	TObjectPtr<USkeletalMeshComponent> AnimatedMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment")
	TObjectPtr<UBoxComponent> InteractionVolume;

	/** Точка на предмете, куда садится линия подсказки. Не центр зоны подхода. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment")
	TObjectPtr<USceneComponent> PromptAnchor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment")
	TObjectPtr<USceneComponent> UseAnchor;

	void PlayUseAnimations(APawn* User);
	void ShowInteractionUI(APawn* User);
	void HideInteractionUI();
	void SetUserMovementLocked(APawn* User, bool bLocked) const;

	UPROPERTY(Transient)
	TObjectPtr<APawn> UserPawn;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> ActiveWidget;
};
