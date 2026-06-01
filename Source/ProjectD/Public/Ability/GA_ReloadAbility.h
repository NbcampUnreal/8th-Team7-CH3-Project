#pragma once

#include "CoreMinimal.h"
#include "Ability/GA_GameplayAbilityBase.h"
#include "GA_ReloadAbility.generated.h"

class APDRangedWeaponBase;
class APDWeaponBase;
class APDPlayerCharacter;
class USoundBase;


UCLASS(Blueprintable)
class PROJECTD_API UGA_ReloadAbility : public UGA_GameplayAbilityBase
{
	GENERATED_BODY()

public:
	UGA_ReloadAbility();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility, bool bWasCancelled) override;

	UPROPERTY(EditDefaultsOnly, Category="Reload")
	TObjectPtr<USoundBase> ReloadSound;

private:
	TWeakObjectPtr<APDRangedWeaponBase> WeaponPtr;

	UFUNCTION()
	void OnWeaponReloaded(APDWeaponBase* Weapon);

	void ExecuteReloadSoundCue(APDPlayerCharacter* Character, APDRangedWeaponBase* Weapon);
};
