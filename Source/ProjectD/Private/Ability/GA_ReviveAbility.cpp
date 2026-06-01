#include "Ability/GA_ReviveAbility.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "AttributeSet/PDAttributeSet.h"
#include "Characters/Base/PDCharacterBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Core/PDPlayerController.h"
#include "GameplayEffect.h"
#include "GameplayTag/PDGameplayTags.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	APDPlayerController* GetReviverPC(APDCharacterBase* Reviver)
	{
		return Reviver ? Cast<APDPlayerController>(Reviver->GetController()) : nullptr;
	}
}

UGA_ReviveAbility::UGA_ReviveAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	ActivationOwnedTags.AddTag(PDGameplayTags::State_Reviving);

	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = PDGameplayTags::Event_Revive_Start;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);

	static ConstructorHelpers::FClassFinder<UGameplayEffect> ReviveEffectFinder(
		TEXT("/Game/Main/Blueprints/Ability/GE/Survival/Revive/GE_Revive"));
	if (ReviveEffectFinder.Succeeded())
	{
		ReviveEffectClass = ReviveEffectFinder.Class;
	}
}

void UGA_ReviveAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	APDCharacterBase* Reviver = GetPDCharacter();
	ReviverCharacter = Reviver;
	ReviveTarget = TriggerEventData
		? Cast<APDCharacterBase>(const_cast<AActor*>(TriggerEventData->Target.Get()))
		: nullptr;
	ReviveStartLocation = Reviver ? Reviver->GetActorLocation() : FVector::ZeroVector;
	bReviveCompleted = false;

	if (!Reviver || !Reviver->HasAuthority())
	{
		PlayReviveLoopMontage();

		UAbilityTask_WaitGameplayEvent* CancelEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, PDGameplayTags::Event_Revive_Cancel);
		CancelEventTask->EventReceived.AddDynamic(this, &UGA_ReviveAbility::OnReviveCancelEvent);
		CancelEventTask->ReadyForActivation();

		const float ClientReviveDuration = GetReviveDuration();
		UAbilityTask_WaitDelay* DelayTask = UAbilityTask_WaitDelay::WaitDelay(this, ClientReviveDuration);
		DelayTask->OnFinish.AddDynamic(this, &UGA_ReviveAbility::OnReviveDelayFinished);
		DelayTask->ReadyForActivation();
		return;
	}

	if (!IsValidRevivePair())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CanContinueRevive())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const float ReviveDuration = GetReviveDuration();
	ReviveTarget->BeginReviveDisplay(Reviver, ReviveDuration);

	// Reviver 측 클라이언트에 HUD CircularProgress(WBP_ReviveProgress) 시작 알림.
	// 위젯은 ReviveTarget 위치를 따라다니므로 Target 동봉.
	if (APDPlayerController* ReviverPC = GetReviverPC(Reviver))
	{
		ReviverPC->Client_NotifyReviveStarted(ReviveTarget, ReviveDuration);
	}

	PlayReviveLoopMontage();

	UAbilityTask_WaitGameplayEvent* CancelEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, PDGameplayTags::Event_Revive_Cancel);
	CancelEventTask->EventReceived.AddDynamic(this, &UGA_ReviveAbility::OnReviveCancelEvent);
	CancelEventTask->ReadyForActivation();

	ScheduleReviveValidation();

	UAbilityTask_WaitDelay* DelayTask = UAbilityTask_WaitDelay::WaitDelay(this, ReviveDuration);
	DelayTask->OnFinish.AddDynamic(this, &UGA_ReviveAbility::OnReviveDelayFinished);
	DelayTask->ReadyForActivation();
}

void UGA_ReviveAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (ReviveTarget && ReviveTarget->HasAuthority())
	{
		ReviveTarget->ClearReviveDisplay();
	}

	// Reviver 측 클라이언트에 HUD CircularProgress 종료 알림 (성공/취소 둘 다). 서버에서만 발사.
	if (ReviverCharacter && ReviverCharacter->HasAuthority())
	{
		if (APDPlayerController* ReviverPC = GetReviverPC(ReviverCharacter))
		{
			ReviverPC->Client_NotifyReviveEnded(!bWasCancelled);
		}
	}

	StopReviveMontages(bWasCancelled ? 0.1f : 0.15f);

	ReviveTarget = nullptr;
	ReviverCharacter = nullptr;
	ReviveStartLocation = FVector::ZeroVector;
	bReviveCompleted = false;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_ReviveAbility::OnReviveDelayFinished()
{
	if (ReviveTarget && ReviveTarget->HasAuthority())
	{
		if (!CanContinueRevive())
		{
			EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
			return;
		}

		CompleteRevive();
		bReviveCompleted = ReviveTarget && !ReviveTarget->IsDowned();
		if (!bReviveCompleted)
		{
			EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
			return;
		}
	}

	StopReviveMontages(0.1f);

	const float CompleteDuration = PlayReviveCompleteMontage();

	if (CompleteDuration > KINDA_SMALL_NUMBER)
	{
		UAbilityTask_WaitDelay* CompleteDelayTask = UAbilityTask_WaitDelay::WaitDelay(this, CompleteDuration);
		CompleteDelayTask->OnFinish.AddDynamic(this, &UGA_ReviveAbility::OnReviveCompleteDelayFinished);
		CompleteDelayTask->ReadyForActivation();
		return;
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_ReviveAbility::OnReviveCompleteDelayFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_ReviveAbility::OnReviveCancelEvent(FGameplayEventData Payload)
{
	if (ReviveTarget && Payload.Target.Get() != ReviveTarget)
	{
		return;
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_ReviveAbility::OnReviveValidationTick()
{
	if (!ReviveTarget || !ReviveTarget->HasAuthority())
	{
		return;
	}

	if (bReviveCompleted)
	{
		return;
	}

	if (!CanContinueRevive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	ScheduleReviveValidation();
}

UAnimInstance* UGA_ReviveAbility::GetReviverAnimInstance() const
{
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	if (!ActorInfo || !ActorInfo->SkeletalMeshComponent.IsValid())
	{
		return nullptr;
	}

	UAnimInstance* AnimInstance = ActorInfo->SkeletalMeshComponent->GetAnimInstance();
	return AnimInstance;
}

float UGA_ReviveAbility::GetReviveDuration() const
{
	return FMath::Max(0.f, ReviveTime) + FMath::Max(0.f, ReviveExtraTime);
}

bool UGA_ReviveAbility::IsValidRevivePair() const
{
	if (!ReviveTarget || !ReviverCharacter || ReviveTarget == ReviverCharacter)
	{
		return false;
	}

	if (!ReviveTarget->IsDowned())
	{
		return false;
	}

	if (ReviveTarget->IsBeingRevived() && ReviveTarget->GetActiveReviver() != ReviverCharacter)
	{
		return false;
	}

	if (ReviverCharacter->IsDowned() || ReviverCharacter->IsGettingUp() || ReviverCharacter->IsDead())
	{
		return false;
	}

	const float DistanceSq = FVector::DistSquared2D(ReviveTarget->GetActorLocation(), ReviverCharacter->GetActorLocation());
	return DistanceSq <= FMath::Square(ReviveInteractDistance);
}

bool UGA_ReviveAbility::CanContinueRevive() const
{
	if (!IsValidRevivePair())
	{
		return false;
	}

	const float MoveTolerance = ReviveCancelMoveTolerance;
	if (MoveTolerance <= KINDA_SMALL_NUMBER)
	{
		return true;
	}

	const float MoveDistanceSq = FVector::DistSquared2D(ReviveStartLocation, ReviverCharacter->GetActorLocation());
	const bool bWithinTolerance = MoveDistanceSq <= FMath::Square(MoveTolerance);
	return bWithinTolerance;
}

void UGA_ReviveAbility::ApplyReviveHealth() const
{
	if (!ReviveTarget || !ReviveTarget->HasAuthority())
	{
		return;
	}

	UAbilitySystemComponent* TargetASC = ReviveTarget->GetAbilitySystemComponent();
	if (!TargetASC)
	{
		return;
	}

	if (ReviveEffectClass)
	{
		FGameplayEffectContextHandle Context = TargetASC->MakeEffectContext();
		Context.AddInstigator(ReviverCharacter, ReviverCharacter);
		FGameplayEffectSpecHandle Spec = TargetASC->MakeOutgoingSpec(ReviveEffectClass, 1.f, Context);
		if (Spec.IsValid())
		{
			TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}
}

void UGA_ReviveAbility::EnsureCriticalHealthForRevive() const
{
	if (!ReviveTarget || !ReviveTarget->HasAuthority())
	{
		return;
	}

	UAbilitySystemComponent* TargetASC = ReviveTarget->GetAbilitySystemComponent();
	if (!TargetASC)
	{
		return;
	}

	const UPDAttributeSet* AttributeSet = TargetASC->GetSet<UPDAttributeSet>();
	if (!AttributeSet)
	{
		return;
	}

	const float MinimumCriticalHealth = FMath::Max(0.f, ReviveMinimumCriticalHealth);
	if (MinimumCriticalHealth <= 0.f)
	{
		return;
	}

	if (AttributeSet->GetHeadHP() <= 0.f)
	{
		TargetASC->SetNumericAttributeBase(
			UPDAttributeSet::GetHeadHPAttribute(),
			FMath::Min(MinimumCriticalHealth, AttributeSet->GetMaxHeadHP()));
	}

	if (AttributeSet->GetTorsoHP() <= 0.f)
	{
		TargetASC->SetNumericAttributeBase(
			UPDAttributeSet::GetTorsoHPAttribute(),
			FMath::Min(MinimumCriticalHealth, AttributeSet->GetMaxTorsoHP()));
	}
}

void UGA_ReviveAbility::CompleteRevive()
{
	ApplyReviveHealth();
	EnsureCriticalHealthForRevive();
	if (ReviveTarget)
	{
		ReviveTarget->BeginGettingUp(ReviverCharacter);
	}
}

void UGA_ReviveAbility::ScheduleReviveValidation()
{
	if (!ReviveTarget || !ReviveTarget->HasAuthority())
	{
		return;
	}

	const float Interval = FMath::Max(0.01f, ReviveValidationInterval);
	UAbilityTask_WaitDelay* ValidationTask = UAbilityTask_WaitDelay::WaitDelay(this, Interval);
	ValidationTask->OnFinish.AddDynamic(this, &UGA_ReviveAbility::OnReviveValidationTick);
	ValidationTask->ReadyForActivation();
}

void UGA_ReviveAbility::PlayReviveLoopMontage()
{
	UAnimInstance* AnimInstance = GetReviverAnimInstance();
	if (!AnimInstance || !ReviveLoopMontage)
	{
		return;
	}

	AnimInstance->Montage_Play(ReviveLoopMontage, MontagePlayRate);
}

float UGA_ReviveAbility::PlayReviveCompleteMontage()
{
	UAnimInstance* AnimInstance = GetReviverAnimInstance();
	if (!AnimInstance || !ReviveCompleteMontage)
	{
		return 0.f;
	}

	AnimInstance->Montage_Play(ReviveCompleteMontage, MontagePlayRate);

	if (ReviveCompleteDuration > KINDA_SMALL_NUMBER)
	{
		return ReviveCompleteDuration;
	}

	return MontagePlayRate > KINDA_SMALL_NUMBER
		? ReviveCompleteMontage->GetPlayLength() / MontagePlayRate
		: ReviveCompleteMontage->GetPlayLength();
}

void UGA_ReviveAbility::StopReviveMontages(float BlendOutTime)
{
	if (UAnimInstance* AnimInstance = GetReviverAnimInstance())
	{
		if (ReviveLoopMontage && AnimInstance->Montage_IsPlaying(ReviveLoopMontage))
		{
			AnimInstance->Montage_Stop(BlendOutTime, ReviveLoopMontage);
		}

		if (ReviveCompleteMontage && AnimInstance->Montage_IsPlaying(ReviveCompleteMontage))
		{
			AnimInstance->Montage_Stop(BlendOutTime, ReviveCompleteMontage);
		}
	}
}
