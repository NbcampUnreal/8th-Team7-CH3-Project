#include "Ability/GA_RollAbility.h"
#include "Characters/Base/PDCharacterBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayTag/PDGameplayTags.h"
#include "TimerManager.h"

UGA_RollAbility::UGA_RollAbility()
{
	InstancingPolicy=EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy=EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	ActivationOwnedTags.AddTag(PDGameplayTags::State_Rolling);
	ActivationBlockedTags.AddTag(PDGameplayTags::State_Rolling);
}

void UGA_RollAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	APDCharacterBase* Character=GetPDCharacter();
	UAbilitySystemComponent* ASC=GetAbilitySystemComponentFromActorInfo();

	if (!Character || Character->IsDowned() || Character->IsGettingUp() || Character->IsDead())
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
	if (UCharacterMovementComponent* MovementComponent = Character->GetCharacterMovement())
	{
		MovementComponent->SetMovementMode(MOVE_Walking);
	}

	if (!RollDir.IsNearlyZero())
	{
		const FRotator RollRotation(0.f, RollDir.Rotation().Yaw, 0.f);
		Character->SetActorRotation(RollRotation);
	}

	if (ASC && RollSound)
	{
		FGameplayCueParameters Params;
		Params.Location = Character->GetActorLocation();
		Params.TargetAttachComponent = Character->GetMesh();
		Params.SourceObject = RollSound;
		ASC->ExecuteGameplayCue(PDGameplayTags::GameplayCue_Character_Roll, Params);
	}

	if (UWorld* World = Character->GetWorld())
	{
		World->GetTimerManager().ClearTimer(AutoFinishRollTimerHandle);
		World->GetTimerManager().SetTimer(
			AutoFinishRollTimerHandle,
			this,
			&UGA_RollAbility::AutoFinishRoll,
			MaxRollDuration,
			false);
	}

	BP_OnActivate(RollDir);
}

void UGA_RollAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	if (const APDCharacterBase* Character = GetPDCharacter())
	{
		if (UWorld* World = Character->GetWorld())
		{
			World->GetTimerManager().ClearTimer(AutoFinishRollTimerHandle);
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}


void UGA_RollAbility::FinishRoll()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}


void UGA_RollAbility::AutoFinishRoll()
{
	FinishRoll();
}


FVector UGA_RollAbility::GetRollDirection() const
{
	APDCharacterBase* Character=GetPDCharacter();
	if (!Character) return FVector::ForwardVector;

	if (bUseInputDirection)
	{
		const FVector Input=Character->GetCharacterMovement()->GetLastInputVector();
		if (!Input.IsNearlyZero())
			return Input.GetSafeNormal2D();
	}

	const FVector Forward = Character->GetActorForwardVector().GetSafeNormal2D();
	return Forward.IsNearlyZero() ? FVector::ForwardVector : Forward;
}
