#include "Ability/GA_MeleeAttackAbility.h"
#include "Characters/Base/PDCharacterBase.h"
#include "Weapons/Base/PDWeaponBase.h"
#include "Weapons/Base/PDMeleeWeaponBase.h"
#include "Interfaces/PDDamageable.h"
#include "GameplayTag/PDGameplayTags.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"

namespace
{
	int32 GetMeleeHitScore(const FHitResult& Hit)
	{
		int32 Score=0;
		if (Hit.bBlockingHit) Score+=1;
		if (!Hit.MyBoneName.IsNone()) Score+=2;
		if (!Hit.BoneName.IsNone()) Score+=4;
		if (Hit.GetComponent() && Hit.GetComponent()->IsA<USkeletalMeshComponent>()) Score+=8;
		return Score;
	}
}

UGA_MeleeAttackAbility::UGA_MeleeAttackAbility()
{
	InstancingPolicy=EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGA_MeleeAttackAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
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

	if (!AttackMontage || AttackSections.IsEmpty())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	HitActors.Reset();


	APDCharacterBase* SwingChar = GetPDCharacter();
	if (SwingChar)
	{
		if (UAbilitySystemComponent* ASCComp = GetAbilitySystemComponentFromActorInfo())
		{
			FGameplayCueParameters Params;
			Params.Location = SwingChar->GetActorLocation();
			Params.TargetAttachComponent = SwingChar->GetMesh();
			Params.SourceObject = SwingChar->GetCurrentWeapon();
			Params.EffectCauser = SwingChar->GetCurrentWeapon();
			Params.Instigator = SwingChar;
			ASCComp->ExecuteGameplayCue(PDGameplayTags::GameplayCue_Weapon_Swing, Params);
		}
	}

	const FName Section=AttackSections[FMath::RandRange(0, AttackSections.Num()-1)];

	UAbilityTask_PlayMontageAndWait* MontageTask=UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, AttackMontage, 1.f, Section);
	MontageTask->OnCompleted.AddDynamic(this, &UGA_MeleeAttackAbility::OnMontageFinished);
	MontageTask->OnBlendOut.AddDynamic(this, &UGA_MeleeAttackAbility::OnMontageFinished);
	MontageTask->OnInterrupted.AddDynamic(this, &UGA_MeleeAttackAbility::OnMontageFinished);
	MontageTask->OnCancelled.AddDynamic(this, &UGA_MeleeAttackAbility::OnMontageFinished);
	MontageTask->ReadyForActivation();

	UAbilityTask_WaitGameplayEvent* EventTask=UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, PDGameplayTags::Anim_Notify_MeleeHit);
	EventTask->EventReceived.AddDynamic(this, &UGA_MeleeAttackAbility::OnMeleeHitReceived);
	EventTask->ReadyForActivation();
}

void UGA_MeleeAttackAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	HitActors.Reset();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_MeleeAttackAbility::OnMeleeHitReceived(FGameplayEventData Payload)
{
	PerformSweep();
}

void UGA_MeleeAttackAbility::OnMontageFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_MeleeAttackAbility::PerformSweep()
{
	APDCharacterBase* Char=GetPDCharacter();
	if (!Char) return;
	if (!Char->HasAuthority()) return;

	APDWeaponBase* WeaponBase=Char->GetCurrentWeapon();
	if (!WeaponBase)
	{
		return;
	}

	APDMeleeWeaponBase* Weapon=Cast<APDMeleeWeaponBase>(WeaponBase);
	if (!Weapon)
	{
		return;
	}

	const float Damage=Weapon->GetCurrentStats().Damage;
	const float SweepRadius=Weapon->GetSweepRadius();
	const float SweepRange=Weapon->GetSweepRange();
	const FName HitSocketName=Weapon->GetHitSocketName();

	const bool bHasHitSocket = Char->GetMesh() && Char->GetMesh()->DoesSocketExist(HitSocketName);
	const FVector Start = bHasHitSocket
		? Char->GetMesh()->GetSocketLocation(HitSocketName)
		: Char->GetActorLocation() + Char->GetActorForwardVector() * 60.f + FVector(0.f, 0.f, 50.f);
	const FVector End=Start+Char->GetActorForwardVector()*SweepRange;


	TArray<FHitResult> Hits;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Char);
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	GetWorld()->SweepMultiByObjectType(Hits, Start, End, FQuat::Identity,
		ObjectParams, FCollisionShape::MakeSphere(SweepRadius), Params);


	bool bPlayedHitSound = false;
	TMap<AActor*, FHitResult> BestHits;
	for (const FHitResult& Hit : Hits)
	{
		AActor* HitActor=Hit.GetActor();
		if (!IsValid(HitActor) || HitActors.Contains(HitActor)) continue;
		if (!HitActor->Implements<UPDDamageable>())
		{
			continue;
		}

		if (FHitResult* ExistingHit=BestHits.Find(HitActor))
		{
			if (GetMeleeHitScore(Hit)>GetMeleeHitScore(*ExistingHit))
			{
				*ExistingHit=Hit;
			}
			continue;
		}

		BestHits.Add(HitActor, Hit);
	}

	for (const auto& Pair : BestHits)
	{
		AActor* HitActor=Pair.Key;
		const FHitResult& Hit=Pair.Value;
		if (!IsValid(HitActor)) continue;

		HitActors.Add(HitActor);


		if (!bPlayedHitSound)
		{
			if (UAbilitySystemComponent* ASCComp = GetAbilitySystemComponentFromActorInfo())
			{
				FGameplayCueParameters CueParams;
				CueParams.Location = Hit.ImpactPoint;
				CueParams.Normal   = Hit.ImpactNormal;
				CueParams.SourceObject = Weapon;
				CueParams.EffectCauser = Weapon;
				CueParams.Instigator = Char;
				ASCComp->ExecuteGameplayCue(PDGameplayTags::GameplayCue_Weapon_MeleeHit, CueParams);
			}
			bPlayedHitSound = true;
		}

		FPDDamageInfo DamageInfo;
		DamageInfo.BaseDamage=Damage;
		DamageInfo.Instigator=Char;
		DamageInfo.HitResult=Hit;
		IPDDamageable::Execute_ApplyDamage(HitActor, DamageInfo);
	}
}
