#include "Ability/GA_PassiveArmInjury.h"
#include "GameplayTag/PDGameplayTags.h"

UGA_PassiveArmInjury::UGA_PassiveArmInjury()
{
	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag=PDGameplayTags::State_Debuff_ArmDamaged;
	TriggerData.TriggerSource=EGameplayAbilityTriggerSource::OwnedTagPresent;
	AbilityTriggers.Add(TriggerData);

	InstancingPolicy=EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGA_PassiveArmInjury::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (InjuryMontage)
	{
		PlayAbilityMontage(InjuryMontage);
	}
}


