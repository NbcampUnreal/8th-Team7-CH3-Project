#pragma once

#include "CoreMinimal.h"
#include "GA_GameplayAbilityBase.h"
#include "GA_PassiveArmInjury.generated.h"

UCLASS()
class PROJECTD_API UGA_PassiveArmInjury : public UGA_GameplayAbilityBase
{
	GENERATED_BODY()

public:
	UGA_PassiveArmInjury();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "PD|Ability")
	TObjectPtr<UAnimMontage> InjuryMontage;
};
