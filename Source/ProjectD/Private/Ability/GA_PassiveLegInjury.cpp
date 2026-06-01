#include "Ability/GA_PassiveLegInjury.h"

#include "GameplayTag/PDGameplayTags.h"

UGA_PassiveLegInjury::UGA_PassiveLegInjury()
{
	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag=PDGameplayTags::State_Debuff_LegDamaged;
	TriggerData.TriggerSource=EGameplayAbilityTriggerSource::OwnedTagPresent;
	AbilityTriggers.Add(TriggerData);

	InstancingPolicy=EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGA_PassiveLegInjury::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	//LoopinSound
	//절뚝거리는 모션
	if (InjuryMontage)
	{
		PlayAbilityMontage(InjuryMontage);
	}
}

