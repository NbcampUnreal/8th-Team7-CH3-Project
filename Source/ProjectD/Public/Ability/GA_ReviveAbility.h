#pragma once

#include "CoreMinimal.h"
#include "Ability/GA_GameplayAbilityBase.h"
#include "GA_ReviveAbility.generated.h"

class APDCharacterBase;
class UAnimInstance;
class UAnimMontage;
class UGameplayEffect;

UCLASS()
class PROJECTD_API UGA_ReviveAbility : public UGA_GameplayAbilityBase
{
	GENERATED_BODY()

public:
	UGA_ReviveAbility();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Revive")
	TObjectPtr<UAnimMontage> ReviveLoopMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Revive")
	TObjectPtr<UAnimMontage> ReviveCompleteMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Revive", meta=(ClampMin="0.01"))
	float MontagePlayRate = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Revive", meta=(ClampMin="0.0", ForceUnits="s"))
	float ReviveCompleteDuration = 0.35f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Revive", meta=(ClampMin="0.0", ForceUnits="s"))
	float ReviveTime = 3.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Revive", meta=(ClampMin="0.0", ForceUnits="s"))
	float ReviveExtraTime = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Revive", meta=(ClampMin="0.0"))
	float ReviveInteractDistance = 300.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Revive", meta=(ClampMin="0.01", ForceUnits="s"))
	float ReviveValidationInterval = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Revive", meta=(ClampMin="0.0"))
	float ReviveCancelMoveTolerance = 25.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Revive")
	TSubclassOf<UGameplayEffect> ReviveEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Revive", meta=(ClampMin="0.0"))
	float ReviveMinimumCriticalHealth = 1.f;

private:
	UFUNCTION()
	void OnReviveDelayFinished();

	UFUNCTION()
	void OnReviveCompleteDelayFinished();

	UFUNCTION()
	void OnReviveCancelEvent(FGameplayEventData Payload);

	UFUNCTION()
	void OnReviveValidationTick();

	UAnimInstance* GetReviverAnimInstance() const;
	float GetReviveDuration() const;
	bool IsValidRevivePair() const;
	bool CanContinueRevive() const;
	void ApplyReviveHealth() const;
	void EnsureCriticalHealthForRevive() const;
	void CompleteRevive();
	void ScheduleReviveValidation();
	void PlayReviveLoopMontage();
	float PlayReviveCompleteMontage();
	void StopReviveMontages(float BlendOutTime = 0.15f);

	UPROPERTY()
	TObjectPtr<APDCharacterBase> ReviveTarget;

	UPROPERTY()
	TObjectPtr<APDCharacterBase> ReviverCharacter;

	FVector ReviveStartLocation = FVector::ZeroVector;

	bool bReviveCompleted = false;
};
