#pragma once

#include "CoreMinimal.h"
#include "Characters/Base/PDCharacterBase.h"
#include "Interfaces/PDSurvivalSource.h"
#include "Type/Types.h"
#include "GameplayEffectTypes.h"
#include "PDPlayerCharacter.generated.h"

class APDWeaponBase;
class UPDVisionComponent;
class UPDInteractionComponent;
class UPDQuickSlotComponent;
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

protected:
	UPROPERTY(EditDefaultsOnly, Category="PD|Survival")
	TSubclassOf<UGameplayEffect> HungerDecayEffectClass;

	UPROPERTY(EditDefaultsOnly, Category="PD|Survival")
	TSubclassOf<UGameplayEffect> ThirstDecayEffectClass;

	UPROPERTY(EditDefaultsOnly, Category="PD|Survival")
	TSubclassOf<UGameplayEffect> StarvingEffectClass;

	UPROPERTY(EditDefaultsOnly, Category="PD|Survival")
	TSubclassOf<UGameplayEffect> DehydratedEffectClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="PD|Player|Weapon")
	TArray<TObjectPtr<APDWeaponBase>> WeaponSlots;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="PD|Player|Weapon")
	EWeaponSlot CurrentSlot=EWeaponSlot::None;

	void OnStaminaChanged(const FOnAttributeChangeData& Data);
	EWeaponSlot GetSlotForWeaponType(EWeaponType Type) const;

public:
	UPROPERTY(BlueprintAssignable, Category="PD|Player|Events")
	FOnWeaponSwapped OnWeaponSwapped;

	UPROPERTY(BlueprintAssignable, Category="PD|Player|Events")
	FOnWeaponPickedUp OnWeaponPickedUp;

	UFUNCTION(BlueprintCallable, Category="PD|Player|Weapon")
	void PickupWeapon(APDWeaponBase* Weapon);

	UFUNCTION(BlueprintCallable, Category="PD|Player|Weapon")
	void SwitchToSlot(EWeaponSlot Slot);

	UFUNCTION(BlueprintCallable, Category="PD|Player|Weapon")
	void DropCurrentWeapon();

	UFUNCTION(BlueprintPure, Category="PD|Player|Weapon")
	APDWeaponBase* GetCurrentWeapon() const;

	UFUNCTION(BlueprintPure, Category="PD|Player|Weapon")
	APDWeaponBase* GetWeaponInSlot(EWeaponSlot Slot) const;

	UFUNCTION(BlueprintPure, Category="PD|Player")
	FORCEINLINE EWeaponSlot GetCurrentSlot() const { return CurrentSlot; }

	UFUNCTION(BlueprintPure, Category="PD|QuickSlot")
	UPDQuickSlotComponent* GetQuickSlotComponent() const { return QuickSlotComponent; }

	UFUNCTION(BlueprintCallable, Category="PD|Interaction")
	void TryInteract();

	virtual TSubclassOf<UGameplayEffect> GetHungerDecayEffectClass()  const override { return HungerDecayEffectClass; }
	virtual TSubclassOf<UGameplayEffect> GetThirstDecayEffectClass()  const override { return ThirstDecayEffectClass; }
	virtual TSubclassOf<UGameplayEffect> GetStarvingEffectClass()     const override { return StarvingEffectClass; }
	virtual TSubclassOf<UGameplayEffect> GetDehydratedEffectClass()   const override { return DehydratedEffectClass; }
public:
	UPROPERTY(EditDefaultsOnly, Category="PD|Player|Animation")
	TMap<FGameplayTag, TSubclassOf<UAnimInstance>> WeaponAnimLayerMap;

	UPROPERTY(EditDefaultsOnly, Category="PD|Player|Animation")
	TSubclassOf<UAnimInstance> DefaultAnimLayerClass;
private:
	void OnWeaponTypeTagChanged(const FGameplayTag Tag, int32 NewCount);
	void LinkDefaultAnimLayer();
};
