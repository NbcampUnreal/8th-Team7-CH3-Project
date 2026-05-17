#include "Ability/PDRollAbility.h"
#include "Characters/Base/PDCharacterBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayTag/PDGameplayTags.h"
#include "Kismet/GameplayStatics.h"

UPDRollAbility::UPDRollAbility()
{
	InstancingPolicy=EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UPDRollAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	APDCharacterBase* Character=GetPDCharacter();
	UAbilitySystemComponent* ASC=GetAbilitySystemComponentFromActorInfo();
	
	if (ASC&&ASC->HasMatchingGameplayTag(PDGameplayTags::State_Rolling))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const FVector RollDir = GetRollDirection();
	if (!RollDir.IsNearlyZero())
		Character->SetActorRotation(RollDir.Rotation());

	if (ASC)
		ASC->AddLooseGameplayTag(PDGameplayTags::State_Rolling);

	if (RollSound)
		UGameplayStatics::SpawnSoundAttached(RollSound, Character->GetMesh());

	BP_OnActivate(RollDir);
}

void UPDRollAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
		ASC->RemoveLooseGameplayTag(PDGameplayTags::State_Rolling);

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}


void UPDRollAbility::FinishRoll()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}


FVector UPDRollAbility::GetRollDirection() const
{
	APDCharacterBase* Character=GetPDCharacter();
	if (!Character) return FVector::ForwardVector;

	if (bUseInputDirection)
	{
		const FVector Input=Character->GetCharacterMovement()->GetLastInputVector();
		if (!Input.IsNearlyZero())
			return Input.GetSafeNormal2D();
	}

	return Character->GetActorForwardVector();
}
