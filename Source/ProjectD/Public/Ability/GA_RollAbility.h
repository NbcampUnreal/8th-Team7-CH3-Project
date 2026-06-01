#pragma once

#include "CoreMinimal.h"
#include "Ability/GA_GameplayAbilityBase.h"
#include "Sound/SoundBase.h"
#include "GA_RollAbility.generated.h"

UCLASS()
class PROJECTD_API UGA_RollAbility : public UGA_GameplayAbilityBase
{
	GENERATED_BODY()

public:
	UGA_RollAbility();

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

	UPROPERTY(EditDefaultsOnly, Category="Roll", meta=(ClampMin="0.05", ForceUnits="s"))
	float MaxRollDuration = 0.9f;

	UPROPERTY(EditDefaultsOnly, Category="Roll")
	TObjectPtr<USoundBase> RollSound;

private:
	FVector GetRollDirection() const;
	void AutoFinishRoll();

	FTimerHandle AutoFinishRollTimerHandle;
};
