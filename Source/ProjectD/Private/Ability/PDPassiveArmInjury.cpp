#include "Ability/PDPassiveArmInjury.h"
#include "GameplayTag/PDGameplayTags.h"

UPDPassiveArmInjury::UPDPassiveArmInjury()
{
	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag=PDGameplayTags::State_Debuff_ArmDamaged;
	TriggerData.TriggerSource=EGameplayAbilityTriggerSource::OwnedTagPresent;
	AbilityTriggers.Add(TriggerData);

	InstancingPolicy=EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UPDPassiveArmInjury::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (InjuryMontage)
	{
		PlayAbilityMontage(InjuryMontage);
	}
}
