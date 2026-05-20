#include "Ability/GA_FireAbility.h"

#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Characters/PDPlayerCharacter.h"
#include "GameplayTag/PDGameplayTags.h"  // Input_FireReleased 이벤트 전용
#include "Weapons/Base/PDRangedWeaponBase.h"

UGA_FireAbility::UGA_FireAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGA_FireAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	APDRangedWeaponBase* Weapon = GetRangedWeapon();
	if (!Weapon || !Weapon->CanFire())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 첫 발 즉시 발사. 판정/탄약/데미지는 서버 무기에서 확정한다.
	if (Weapon->HasAuthority())
	{
		Weapon->Fire();
	}
	else
	{
		Weapon->ServerFire();
	}

	// 자동화기: 무기의 bFullAuto 플래그로 판단
	if (Weapon->IsFullAuto())
	{
		const float FireRate = Weapon->GetCurrentStats().FireRate;
		GetWorld()->GetTimerManager().SetTimer(
			AutoFireHandle,
			this, &UGA_FireAbility::DoAutoFire,
			FireRate, true);
	}

	// Input.FireReleased 이벤트 대기 → 어빌리티 종료
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
	// CanFire()가 false여도 서버 Weapon->Fire() 내부에서 걸러짐 (탄 소진 등)
	if (Weapon->HasAuthority())
	{
		Weapon->Fire();
	}
	else
	{
		Weapon->ServerFire();
	}
}

APDRangedWeaponBase* UGA_FireAbility::GetRangedWeapon() const
{
	APDPlayerCharacter* Char = Cast<APDPlayerCharacter>(GetPDCharacter());
	return Char ? Cast<APDRangedWeaponBase>(Char->GetCurrentWeapon()) : nullptr;
}
