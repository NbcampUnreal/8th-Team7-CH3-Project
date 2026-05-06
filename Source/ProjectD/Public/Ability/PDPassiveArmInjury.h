#pragma once

#include "CoreMinimal.h"
#include "PDGameplayAbilityBase.h"
#include "PDPassiveArmInjury.generated.h"

UCLASS()
class PROJECTD_API UPDPassiveArmInjury : public UPDGameplayAbilityBase
{
	GENERATED_BODY()

public:
	UPDPassiveArmInjury();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "PD|Ability")
	TObjectPtr<UAnimMontage> InjuryMontage;
};
