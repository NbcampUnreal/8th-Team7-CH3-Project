#pragma once

#include "CoreMinimal.h"
#include "Characters/Base/PDCharacterBase.h"
#include "Interfaces/PDSurvivalSource.h"
#include "Type/Types.h"
#include "GameplayEffectTypes.h"
#include "ActiveGameplayEffectHandle.h"
#include "PDPlayerCharacter.generated.h"

class APDWeaponBase;
class APDRangedWeaponBase;
class APDPlayerState;
class UPDAnimInstance;
class UPDVisionComponent;
class UPDInteractionComponent;
class UPDInventoryComponent;
class UPDQuickSlotComponent;
class UPDEquipmentComponent;
class UPDEquipmentModificationComponent;
class UPDQuestComponent;
class UCameraComponent;
class USpringArmComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeaponSwapped, APDWeaponBase*, NewWeapon, EWeaponSlot, Slot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponPickedUp, APDWeaponBase*, Weapon);

UCLASS(abstract, Blueprintable)
class APDPlayerCharacter : public APDCharacterBase,
						   public IPDSurvivalSource
{
	GENERATED_BODY()

public:
	APDPlayerCharacter();
	virtual void InitAbilitySystem() override;

	UCameraComponent* GetTopDownCameraComponent() const { return TopDownCameraComponent.Get(); }
	USpringArmComponent* GetCameraBoom() const { return CameraBoom.Get(); }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void HandleDeath(AActor* Killer) override;
	virtual bool ShouldEnterDownedStateOnLethalDamage() const override { return true; }
	virtual void OnLifeStateChanged(EPDLifeState OldLifeState, AActor* ContextActor) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UCameraComponent> TopDownCameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UPDVisionComponent> VisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UPDInteractionComponent> InteractionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UPDEquipmentModificationComponent> EquipmentModificationComponent;

protected:
	UPROPERTY(EditDefaultsOnly, Category="PD|Stamina")
	TSubclassOf<UGameplayEffect> StaminaRegenEffectClass;

	UPROPERTY(EditDefaultsOnly, Category="PD|Stamina")
	TSubclassOf<UGameplayEffect> StaminaRegenBonusEffectClass;

	UPROPERTY(EditDefaultsOnly, Category="PD|Survival")
	TSubclassOf<UGameplayEffect> HungerDecayEffectClass;

	UPROPERTY(EditDefaultsOnly, Category="PD|Survival")
	TSubclassOf<UGameplayEffect> ThirstDecayEffectClass;

	UPROPERTY(EditDefaultsOnly, Category="PD|Survival")
	TSubclassOf<UGameplayEffect> StarvingEffectClass;

	UPROPERTY(EditDefaultsOnly, Category="PD|Survival")
	TSubclassOf<UGameplayEffect> DehydratedEffectClass;

	UPROPERTY(EditDefaultsOnly, Category="PD|Survival")
	TSubclassOf<UGameplayEffect> GasMaskDecayEffectClass;

	UPROPERTY(EditDefaultsOnly, Category="PD|Survival")
	TSubclassOf<UGameplayEffect> GasExposureEffectClass;

	UPROPERTY(ReplicatedUsing=OnRep_WeaponSlots, VisibleAnywhere, BlueprintReadOnly, Category="PD|Player|Weapon")
	TArray<TObjectPtr<APDWeaponBase>> WeaponSlots;

	UPROPERTY(ReplicatedUsing=OnRep_CurrentSlot, VisibleAnywhere, BlueprintReadOnly, Category="PD|Player|Weapon")
	EWeaponSlot CurrentSlot=EWeaponSlot::None;

	UPROPERTY(ReplicatedUsing=OnRep_ReplicatedWeaponType, VisibleAnywhere, BlueprintReadOnly, Category="PD|Player|Weapon")
	EWeaponType ReplicatedWeaponType = EWeaponType::None;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="PD|Player|Vision")
	FVector ReplicatedAimWorldLocation = FVector::ZeroVector;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="PD|Player|Vision")
	bool bHasReplicatedAimWorldLocation = false;

	void OnStaminaChanged(const FOnAttributeChangeData& Data);

public:
	UFUNCTION(BlueprintPure, Category="PD|Player|Weapon")
	EWeaponSlot GetSlotForWeaponType(EWeaponType Type) const;

	UPROPERTY(BlueprintAssignable, Category="PD|Player|Events")
	FOnWeaponSwapped OnWeaponSwapped;

	UPROPERTY(BlueprintAssignable, Category="PD|Player|Events")
	FOnWeaponPickedUp OnWeaponPickedUp;

	UFUNCTION(BlueprintCallable, Category="PD|Player|Weapon")
	void PickupWeapon(APDWeaponBase* Weapon);

	void SetSharedAimWorldLocation(const FVector& AimLocation);
	bool GetSharedAimWorldLocation(FVector& OutLocation) const;
	FVector GetSharedVisionForwardVector(const FVector& FromLocation) const;



	UFUNCTION(BlueprintCallable, Category="PD|Player|Weapon")
	bool TryAutoEquipWeaponItem(const FPDItemData& ItemData);



	UFUNCTION(BlueprintCallable, Category="PD|Player|Weapon")
	bool TryAutoEquipWeaponSlot(const FPDInventorySlot& ItemSlot);

	UFUNCTION(BlueprintCallable, Category="PD|Player|Weapon")
	bool RemoveEquippedWeaponItem(const FPDItemData& ItemData, bool bDestroyWeaponActor = true);

	UFUNCTION(BlueprintCallable, Category="PD|Player|Weapon")
	void SwitchToSlot(EWeaponSlot Slot);

	UFUNCTION(BlueprintCallable, Category="PD|Player|Weapon")
	void DropCurrentWeapon();

	virtual APDWeaponBase* GetCurrentWeapon() const override;

	UFUNCTION(BlueprintPure, Category="PD|Player|Weapon")
	APDWeaponBase* GetWeaponInSlot(EWeaponSlot Slot) const;

	UFUNCTION(BlueprintPure, Category="PD|Player")
	FORCEINLINE EWeaponSlot GetCurrentSlot() const { return CurrentSlot; }

	UFUNCTION(BlueprintPure, Category="PD|Player|Weapon")
	FORCEINLINE EWeaponType GetReplicatedWeaponType() const { return ReplicatedWeaponType; }

	UFUNCTION(BlueprintPure, Category="PD|QuickSlot")
	UPDQuickSlotComponent* GetQuickSlotComponent() const;

	UFUNCTION(BlueprintPure, Category="PD|Inventory")
	UPDInventoryComponent* GetInventoryComponent() const;

	UFUNCTION(BlueprintPure, Category="PD|Equipment")
	UPDEquipmentComponent* GetEquipmentComponent() const;

	UFUNCTION(BlueprintPure, Category="PD|Quest")
	UPDQuestComponent* GetQuestComponent() const;

	UFUNCTION(BlueprintPure, Category="PD|Equipment")
	UPDEquipmentModificationComponent* GetEquipmentModificationComponent() const { return EquipmentModificationComponent; }

	UFUNCTION(BlueprintCallable, Category="PD|Interaction")
	void TryInteract();

	UFUNCTION(BlueprintCallable, Category="PD|Interaction")
	void StopInteract();

	bool IsInteractingWith(const AActor* TargetActor) const;

	UFUNCTION(Server, Reliable)
	void ServerTryInteract();

	UFUNCTION(Server, Reliable)
	void ServerInteractTarget(AActor* TargetActor);

	UFUNCTION(Server, Reliable)
	void ServerStopInteract(AActor* TargetActor);

	UFUNCTION(Server, Reliable)
	void ServerHandleAnimGameplayEvent(FGameplayTag EventTag);


	UFUNCTION(BlueprintCallable, Category="PD|Survival")
	void ResetToBase();

	virtual TSubclassOf<UGameplayEffect> GetHungerDecayEffectClass()  const override { return HungerDecayEffectClass; }
	virtual TSubclassOf<UGameplayEffect> GetThirstDecayEffectClass()  const override { return ThirstDecayEffectClass; }
	virtual TSubclassOf<UGameplayEffect> GetStarvingEffectClass()     const override { return StarvingEffectClass; }
	virtual TSubclassOf<UGameplayEffect> GetDehydratedEffectClass()   const override { return DehydratedEffectClass; }
	virtual TSubclassOf<UGameplayEffect> GetGasMaskDecayEffectClass() const override { return GasMaskDecayEffectClass; }
	virtual TSubclassOf<UGameplayEffect> GetGasExposureEffectClass()  const override { return GasExposureEffectClass; }
public:
	UPROPERTY(EditDefaultsOnly, Category="PD|Player|Animation")
	TMap<FGameplayTag, TSubclassOf<UAnimInstance>> WeaponAnimLayerMap;

	UPROPERTY(EditDefaultsOnly, Category="PD|Player|Animation")
	TSubclassOf<UAnimInstance> DefaultAnimLayerClass;

	UPROPERTY(EditDefaultsOnly, Category="PD|Player|Animation")
	TSubclassOf<UAnimInstance> DownedAnimLayerClass;
private:
	UFUNCTION()
	void OnRep_WeaponSlots();

	UFUNCTION()
	void OnRep_CurrentSlot();

	UFUNCTION()
	void OnRep_ReplicatedWeaponType();

	void OnWeaponTypeTagChanged(const FGameplayTag Tag, int32 NewCount);
	void ApplyAnimationLayerForTag(const FGameplayTag& LayerTag);
	void LinkDefaultAnimLayer();
	void LinkDownedAnimLayer();
	void ApplyWeaponAnimationLayer(APDWeaponBase* Weapon);
	void ApplyWeaponAnimationLayerForType(EWeaponType WeaponType);
	void SyncWeaponTypeTags(EWeaponType WeaponType);
	void SyncWeaponPresentation();
	UPDInteractionComponent* GetOrCreateInteractionComponent();
	void BeginGettingUpPresentation();
	void FinishGettingUpFromTimer();

	UFUNCTION(Server, Reliable)
	void ServerPickupWeapon(APDWeaponBase* Weapon);

	UFUNCTION(Server, Reliable)
	void ServerSwitchToSlot(EWeaponSlot Slot);

	UFUNCTION(Server, Reliable)
	void ServerDropCurrentWeapon();

	FActiveGameplayEffectHandle HungerDecayHandle;
	FActiveGameplayEffectHandle ThirstDecayHandle;
	FActiveGameplayEffectHandle GasMaskDecayHandle;

	bool bPlayerAbilityDelegatesBound = false;
	bool bPlayerPersistentEffectsApplied = false;

	TWeakObjectPtr<AActor> ActiveInteractTarget;
	FTimerHandle GetUpTimerHandle;
};
