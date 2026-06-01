#pragma once

#include "CoreMinimal.h"
#include "Ability/GA_GameplayAbilityBase.h"
#include "GameplayEffectTypes.h"
#include "Type/Types.h"
#include "GA_UseConsumableAbility.generated.h"

class UGameplayEffect;
class UPDInventoryComponent;
class UPDQuickSlotComponent;

UCLASS(BlueprintType)
class PROJECTD_API UPDConsumableUseRequest : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TObjectPtr<UPDQuickSlotComponent> QuickSlotComponent = nullptr;

	UPROPERTY()
	FPDInventorySlot ItemSlot;

	UPROPERTY()
	int32 QuickSlotIndex = INDEX_NONE;
};

UCLASS()
class PROJECTD_API UGA_UseConsumableAbility : public UGA_GameplayAbilityBase
{
	GENERATED_BODY()

public:
	UGA_UseConsumableAbility();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Consumable")
	TSubclassOf<UGameplayEffect> UseMoveSpeedEffectClass;

private:
	UPROPERTY()
	TObjectPtr<UPDConsumableUseRequest> ActiveRequest = nullptr;

	FActiveGameplayEffectHandle MoveSpeedEffectHandle;

	UFUNCTION()
	void FinishConsumableUse();

	UFUNCTION()
	void OnCancelConsumableUse(FGameplayEventData Payload);

	UPDInventoryComponent* ResolveInventoryComponent() const;
	bool RemoveConsumableFromInventory(UPDInventoryComponent* InventoryComponent, const FPDInventorySlot& Slot) const;
	void ApplyConsumableEffect(const FPDInventorySlot& Slot);
	void RemoveMoveSpeedEffect();
};
