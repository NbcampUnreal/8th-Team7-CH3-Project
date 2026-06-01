#include "Ability/GA_SprintAbility.h"

#include "AbilitySystemComponent.h"
#include "AttributeSet/PDAttributeSet.h"
#include "Characters/Base/PDCharacterBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayEffect.h"
#include "GameplayTag/PDGameplayTags.h"

UGA_SprintAbility::UGA_SprintAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	FGameplayTagContainer SprintAbilityTags;
	SprintAbilityTags.AddTag(PDGameplayTags::Input_Sprint);
	SetAssetTags(SprintAbilityTags);

	ActivationOwnedTags.AddTag(PDGameplayTags::State_Sprinting);
	ActivationBlockedTags.AddTag(PDGameplayTags::State_Sprinting);
	ActivationBlockedTags.AddTag(PDGameplayTags::State_Rolling);
	ActivationBlockedTags.AddTag(PDGameplayTags::State_Downed);
	ActivationBlockedTags.AddTag(PDGameplayTags::State_GettingUp);
	ActivationBlockedTags.AddTag(PDGameplayTags::State_Dead);
}

bool UGA_SprintAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	return !ASC || !ASC->HasMatchingGameplayTag(PDGameplayTags::State_Sprinting);
}

void UGA_SprintAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	const APDCharacterBase* Character = GetPDCharacter();
	const UPDAttributeSet* AttributeSet = GetAttributeSet();
	if (!Character || Character->IsDowned() || Character->IsGettingUp() || Character->IsDead())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (AttributeSet && AttributeSet->GetStamina() < MinStaminaToStart)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		RemoveSprintGameplayEffects(ASC);
	}

	SprintEffectHandle = ApplySprintGameplayEffect(SprintEffectClass, Handle, ActorInfo, ActivationInfo);
	SprintCostEffectHandle = ApplySprintGameplayEffect(SprintCostEffectClass, Handle, ActorInfo, ActivationInfo);

	if ((!SprintEffectHandle.IsValid() && SprintEffectClass) ||
		(!SprintCostEffectHandle.IsValid() && SprintCostEffectClass))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	BindStaminaChanged();
}

void UGA_SprintAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	UnbindStaminaChanged();

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		if (ASC->IsOwnerActorAuthoritative())
		{
			if (SprintEffectHandle.IsValid())
			{
				ASC->RemoveActiveGameplayEffect(SprintEffectHandle);
			}

			if (SprintCostEffectHandle.IsValid())
			{
				ASC->RemoveActiveGameplayEffect(SprintCostEffectHandle);
			}

			RemoveSprintGameplayEffects(ASC);
		}
	}

	SyncCharacterMovementSpeed();

	SprintEffectHandle.Invalidate();
	SprintCostEffectHandle.Invalidate();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

FActiveGameplayEffectHandle UGA_SprintAbility::ApplySprintGameplayEffect(TSubclassOf<UGameplayEffect> EffectClass,
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo) const
{
	if (!EffectClass)
	{
		return FActiveGameplayEffectHandle();
	}

	return ApplyGameplayEffectToOwner(
		Handle,
		ActorInfo,
		ActivationInfo,
		EffectClass.GetDefaultObject(),
		GetAbilityLevel(Handle, ActorInfo));
}

void UGA_SprintAbility::RemoveSprintGameplayEffects(UAbilitySystemComponent* ASC) const
{
	if (!ASC)
	{
		return;
	}

	if (!ASC->IsOwnerActorAuthoritative())
	{
		return;
	}

	if (SprintEffectClass)
	{
		ASC->RemoveActiveGameplayEffectBySourceEffect(SprintEffectClass, ASC);
	}

	if (SprintCostEffectClass)
	{
		ASC->RemoveActiveGameplayEffectBySourceEffect(SprintCostEffectClass, ASC);
	}
}

void UGA_SprintAbility::SyncCharacterMovementSpeed() const
{
	APDCharacterBase* Character = GetPDCharacter();
	if (!Character || Character->IsDowned() || Character->IsGettingUp() || Character->IsDead())
	{
		return;
	}

	UCharacterMovementComponent* MovementComponent = Character->GetCharacterMovement();
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!MovementComponent || !ASC)
	{
		return;
	}

	MovementComponent->MaxWalkSpeed = ASC->GetNumericAttribute(UPDAttributeSet::GetMoveSpeedAttribute());
}

void UGA_SprintAbility::BindStaminaChanged()
{
	if (!bEndWhenStaminaEmpty || StaminaChangedDelegateHandle.IsValid())
	{
		return;
	}

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		StaminaChangedDelegateHandle = ASC->GetGameplayAttributeValueChangeDelegate(
			UPDAttributeSet::GetStaminaAttribute()).AddUObject(this, &UGA_SprintAbility::OnStaminaChanged);
	}
}

void UGA_SprintAbility::UnbindStaminaChanged()
{
	if (!StaminaChangedDelegateHandle.IsValid())
	{
		return;
	}

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->GetGameplayAttributeValueChangeDelegate(UPDAttributeSet::GetStaminaAttribute())
			.Remove(StaminaChangedDelegateHandle);
	}

	StaminaChangedDelegateHandle.Reset();
}

void UGA_SprintAbility::OnStaminaChanged(const FOnAttributeChangeData& Data)
{
	if (!bEndWhenStaminaEmpty)
	{
		return;
	}

	if (Data.NewValue <= MinStaminaToKeepSprinting)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
	}
}
