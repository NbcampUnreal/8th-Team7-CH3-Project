#pragma once

#include "CoreMinimal.h"
#include "Ability/PDGameplayAbilityBase.h"
#include "PDMeleeAttackAbility.generated.h"

UCLASS()
class PROJECTD_API UPDMeleeAttackAbility : public UPDGameplayAbilityBase
{
	GENERATED_BODY()

public:
	UPDMeleeAttackAbility();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category="Melee")
	TObjectPtr<UAnimMontage> AttackMontage;

	UPROPERTY(EditDefaultsOnly, Category="Melee")
	TArray<FName> AttackSections = {TEXT("Attack1"), TEXT("Attack2"), TEXT("Attack3"), TEXT("Attack4")};


private:
	UFUNCTION()
	void OnMeleeHitReceived(FGameplayEventData Payload);

	UFUNCTION()
	void OnMontageFinished();

	void PerformSweep();

	TSet<AActor*> HitActors;
};
