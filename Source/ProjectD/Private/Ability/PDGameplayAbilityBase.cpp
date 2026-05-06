#include "Ability/PDGameplayAbilityBase.h"
#include "Characters/Base/PDCharacterBase.h"
#include "Core/PDPlayerController.h"
#include "AttributeSet/PDAttributeSet.h"

UPDGameplayAbilityBase::UPDGameplayAbilityBase()
{
	InstancingPolicy=EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

APDCharacterBase* UPDGameplayAbilityBase::GetPDCharacter() const
{
	return Cast<APDCharacterBase>(GetAvatarActorFromActorInfo());
}

APDPlayerController* UPDGameplayAbilityBase::GetPDPlayerController() const
{
	return Cast<APDPlayerController>(GetActorInfo().PlayerController.Get());
}

const UPDAttributeSet* UPDGameplayAbilityBase::GetAttributeSet() const
{
	APDCharacterBase* Character=GetPDCharacter();
	if (!Character) return nullptr;

	UAbilitySystemComponent* ASC=Character->GetAbilitySystemComponent();
	if (!ASC) return nullptr;

	return ASC->GetSet<UPDAttributeSet>();
}

void UPDGameplayAbilityBase::PlayAbilityMontage(UAnimMontage* Montage, float PlayRate)
{
	if (!Montage) return;

	UAbilitySystemComponent* ASC=GetAbilitySystemComponentFromActorInfo();
	if (!ASC) return;

	ASC->PlayMontage(this, GetCurrentActivationInfo(), Montage, PlayRate);
}
