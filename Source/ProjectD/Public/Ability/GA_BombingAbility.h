#pragma once

#include "CoreMinimal.h"
#include "Ability/GA_GameplayAbilityBase.h"
#include "GA_BombingAbility.generated.h"

class AGameplayAbilityTargetActor;
class UGameplayEffect;
class UMaterialInterface;

UCLASS()
class PROJECTD_API UGA_BombingAbility : public UGA_GameplayAbilityBase
{
	GENERATED_BODY()

public:
	UGA_BombingAbility();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category="PD|Bombing")
	TSubclassOf<AGameplayAbilityTargetActor> TargetActorClass;

	UPROPERTY(EditDefaultsOnly, Category="PD|Bombing")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, Category="PD|Bombing", meta=(ClampMin="0.0"))
	float Damage = 80.f;

	UPROPERTY(EditDefaultsOnly, Category="PD|Bombing", meta=(ClampMin="1.0", ForceUnits="cm"))
	float Radius = 450.f;

	UPROPERTY(EditDefaultsOnly, Category="PD|Bombing", meta=(ClampMin="0.0", ForceUnits="cm"))
	float MaxTargetRange = 3000.f;

	UPROPERTY(EditDefaultsOnly, Category="PD|Bombing", meta=(ClampMin="0.0", ForceUnits="s"))
	float ImpactDelay = 0.2f;

	UPROPERTY(EditDefaultsOnly, Category="PD|Bombing")
	TEnumAsByte<ECollisionChannel> TargetTraceChannel = ECC_Visibility;

	UPROPERTY(EditDefaultsOnly, Category="PD|Bombing")
	TObjectPtr<UMaterialInterface> TargetDecalMaterial;

	UPROPERTY(EditDefaultsOnly, Category="PD|Bombing")
	bool bFallbackToDamageableInterface = true;

	UFUNCTION()
	void OnTargetDataReady(const FGameplayAbilityTargetDataHandle& TargetData);

	UFUNCTION()
	void OnTargetDataCancelled(const FGameplayAbilityTargetDataHandle& TargetData);

private:
	void StartTargeting();
	void ConfirmBombingLocation(const FVector& TargetLocation, const FVector& TargetNormal);
	void ExecuteBombingAtLocation(FVector TargetLocation, FVector TargetNormal);
	void ApplyAreaDamage(const FVector& TargetLocation, const FVector& TargetNormal);
	bool ResolveFallbackTarget(FVector& OutLocation, FVector& OutNormal) const;
	FHitResult MakeAreaDamageHit(AActor* HitActor, const FVector& TargetLocation, const FVector& TargetNormal) const;
};
