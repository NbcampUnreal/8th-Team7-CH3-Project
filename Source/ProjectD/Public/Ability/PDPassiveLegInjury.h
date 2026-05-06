// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PDGameplayAbilityBase.h"
#include "PDPassiveLegInjury.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTD_API UPDPassiveLegInjury : public UPDGameplayAbilityBase
{
	GENERATED_BODY()
public:
	UPDPassiveLegInjury();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "PD|Ability")
	TObjectPtr<UAnimMontage> InjuryMontage;

};
