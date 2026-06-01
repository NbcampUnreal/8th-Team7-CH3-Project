// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GA_GameplayAbilityBase.h"
#include "GA_PassiveLegInjury.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTD_API UGA_PassiveLegInjury : public UGA_GameplayAbilityBase
{
	GENERATED_BODY()
public:
	UGA_PassiveLegInjury();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "PD|Ability")
	TObjectPtr<UAnimMontage> InjuryMontage;

};
