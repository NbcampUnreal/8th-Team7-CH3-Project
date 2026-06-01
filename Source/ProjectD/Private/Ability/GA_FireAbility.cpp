#include "Ability/GA_FireAbility.h"

#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Characters/PDPlayerCharacter.h"
#include "GameplayTag/PDGameplayTags.h"
#include "Weapons/Base/PDRangedWeaponBase.h"

UGA_FireAbility::UGA_FireAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void UGA_FireAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	APDRangedWeaponBase* Weapon = GetRangedWeapon();

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!Weapon)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const bool bCanFire = Weapon->CanFire();

	if (!bCanFire)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}


	Weapon->Fire();


	if (Weapon->IsFullAuto())
	{
		const float FireRate = Weapon->GetCurrentStats().FireRate;
		GetWorld()->GetTimerManager().SetTimer(
			AutoFireHandle,
			this, &UGA_FireAbility::DoAutoFire,
			FireRate, true);
	}


	UAbilityTask_WaitGameplayEvent* EventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, PDGameplayTags::Input_FireReleased);
	EventTask->EventReceived.AddDynamic(this, &UGA_FireAbility::OnFireReleasedEvent);
	EventTask->ReadyForActivation();
}

void UGA_FireAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	GetWorld()->GetTimerManager().ClearTimer(AutoFireHandle);
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_FireAbility::OnFireReleasedEvent(FGameplayEventData Payload)
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_FireAbility::DoAutoFire()
{
	APDRangedWeaponBase* Weapon = GetRangedWeapon();
	if (!Weapon)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	Weapon->Fire();
}

APDRangedWeaponBase* UGA_FireAbility::GetRangedWeapon() const
{
	APDPlayerCharacter* Char = Cast<APDPlayerCharacter>(GetPDCharacter());
	return Char ? Cast<APDRangedWeaponBase>(Char->GetCurrentWeapon()) : nullptr;
}
