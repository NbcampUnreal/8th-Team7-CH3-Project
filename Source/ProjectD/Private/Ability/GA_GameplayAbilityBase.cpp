#include "Ability/GA_GameplayAbilityBase.h"
#include "Characters/Base/PDCharacterBase.h"
#include "Core/PDPlayerController.h"
#include "AttributeSet/PDAttributeSet.h"

UGA_GameplayAbilityBase::UGA_GameplayAbilityBase()
{
	InstancingPolicy=EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

APDCharacterBase* UGA_GameplayAbilityBase::GetPDCharacter() const
{
	return Cast<APDCharacterBase>(GetAvatarActorFromActorInfo());
}

// APDPlayerController* UGA_GameplayAbilityBase::GetPDPlayerController() const
// {
// 	return Cast<APDPlayerController>(GetActorInfo().PlayerController.Get());
// }

const UPDAttributeSet* UGA_GameplayAbilityBase::GetAttributeSet() const
{
	APDCharacterBase* Character=GetPDCharacter();
	if (!Character) return nullptr;

	UAbilitySystemComponent* ASC=Character->GetAbilitySystemComponent();
	if (!ASC) return nullptr;

	return ASC->GetSet<UPDAttributeSet>();
}

void UGA_GameplayAbilityBase::PlayAbilityMontage(UAnimMontage* Montage, float PlayRate)
{
	if (!Montage) return;

	UAbilitySystemComponent* ASC=GetAbilitySystemComponentFromActorInfo();
	if (!ASC) return;

	ASC->PlayMontage(this, GetCurrentActivationInfo(), Montage, PlayRate);
}
