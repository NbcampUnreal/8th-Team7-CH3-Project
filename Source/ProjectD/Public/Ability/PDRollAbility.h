#pragma once

#include "CoreMinimal.h"
#include "Ability/PDGameplayAbilityBase.h"
#include "Sound/SoundBase.h"
#include "PDRollAbility.generated.h"

UCLASS()
class PROJECTD_API UPDRollAbility : public UPDGameplayAbilityBase
{
	GENERATED_BODY()

public:
	UPDRollAbility();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility, bool bWasCancelled) override;
	
	UFUNCTION(BlueprintImplementableEvent, Category="PD|Roll")
	void BP_OnActivate(const FVector& RollDirection);
	
	UFUNCTION(BlueprintCallable, Category="PD|Roll")
	void FinishRoll();

protected:
	UPROPERTY(EditDefaultsOnly, Category="Roll")
	bool bUseInputDirection = true;

	UPROPERTY(EditDefaultsOnly, Category="Roll")
	TObjectPtr<USoundBase> RollSound;

private:
	FVector GetRollDirection() const;
};
