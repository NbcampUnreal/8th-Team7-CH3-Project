#pragma once

#include "CoreMinimal.h"
#include "Characters/Base/PDCharacterBase.h"
#include "Interfaces/PDSurvivalSource.h"
#include "Type/Types.h"
#include "GameplayEffectTypes.h"
#include "ActiveGameplayEffectHandle.h"
#include "Cover/PDCoverBase.h"
#include "PDPlayerCharacter.generated.h"

class APDWeaponBase;
class APDRangedWeaponBase;
class UPDAnimInstance;
class UPDVisionComponent;
class UPDInteractionComponent;
class UPDQuickSlotComponent;
class UPDEquipmentComponent;
class UPDEquipmentModificationComponent;
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
	TObjectPtr<UPDQuickSlotComponent> QuickSlotComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UPDEquipmentComponent> EquipmentComponent;

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="PD|Player|Weapon")
	TArray<TObjectPtr<APDWeaponBase>> WeaponSlots;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="PD|Player|Weapon")
	EWeaponSlot CurrentSlot=EWeaponSlot::None;

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

	// 인벤토리/박스의 무기 아이템 데이터를 받아서 빈 슬롯이 있으면 spawn 후 자동 장착.
	// 슬롯이 차 있거나 WeaponClass가 없으면 false 반환(호출자가 인벤토리로 보내야 함).
	UFUNCTION(BlueprintCallable, Category="PD|Player|Weapon")
	bool TryAutoEquipWeaponItem(const FPDItemData& ItemData);

	// 개조 레벨 등 개별 슬롯 상태를 유지하면서 무기를 장착한다.
	// 장착된 무기의 기존 레벨 시스템에는 ModificationLevel + 1을 전달한다.
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

	UFUNCTION(BlueprintPure, Category="PD|QuickSlot")
	UPDQuickSlotComponent* GetQuickSlotComponent() const { return QuickSlotComponent; }

	UFUNCTION(BlueprintPure, Category="PD|Equipment")
	UPDEquipmentComponent* GetEquipmentComponent() const { return EquipmentComponent; }

	UFUNCTION(BlueprintPure, Category="PD|Equipment")
	UPDEquipmentModificationComponent* GetEquipmentModificationComponent() const { return EquipmentModificationComponent; }

	UFUNCTION(BlueprintCallable, Category="PD|Interaction")
	void TryInteract();

	//기지진입하면 모든 디버프, AS초기화
	UFUNCTION(BlueprintCallable, Category="PD|Survival")
	void ResetToBase();

	UFUNCTION(BlueprintPure, Category="PD|Cover")
	APDCoverBase* GetCoverCandidate() const { return CoverCandidate.Get(); }
	void SetCoverCandidate(APDCoverBase* Cover) { CoverCandidate = Cover; }

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
private:
	void OnWeaponTypeTagChanged(const FGameplayTag Tag, int32 NewCount);
	void LinkDefaultAnimLayer();

	FActiveGameplayEffectHandle HungerDecayHandle;
	FActiveGameplayEffectHandle ThirstDecayHandle;
	FActiveGameplayEffectHandle GasMaskDecayHandle;

	TWeakObjectPtr<APDCoverBase> CoverCandidate;
};
