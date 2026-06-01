#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTargetActor.h"
#include "PDGroundTargetActor.generated.h"

class UDecalComponent;
class UMaterialInterface;

UCLASS(Blueprintable)
class PROJECTD_API APDGroundTargetActor : public AGameplayAbilityTargetActor
{
	GENERATED_BODY()

public:
	APDGroundTargetActor();

	virtual void StartTargeting(UGameplayAbility* Ability) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual bool IsConfirmTargetingAllowed() override;
	virtual void ConfirmTargetingAndContinue() override;
	virtual void CancelTargeting() override;

	UFUNCTION(BlueprintCallable, Category="PD|Targeting")
	void SetTargetRadius(float InRadius);

	UFUNCTION(BlueprintCallable, Category="PD|Targeting")
	void SetMaxTargetRange(float InRange);

	UFUNCTION(BlueprintCallable, Category="PD|Targeting")
	void SetTraceChannel(TEnumAsByte<ECollisionChannel> InTraceChannel);

	UFUNCTION(BlueprintCallable, Category="PD|Targeting")
	void SetDecalMaterial(UMaterialInterface* InMaterial);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="PD|Targeting")
	TObjectPtr<UDecalComponent> TargetDecal;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Targeting", meta=(ClampMin="1.0", ForceUnits="cm"))
	float TargetRadius = 450.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Targeting", meta=(ClampMin="0.0", ForceUnits="cm"))
	float MaxTargetRange = 3000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Targeting", meta=(ClampMin="1.0", ForceUnits="cm"))
	float DecalDepth = 256.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Targeting")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Targeting")
	TObjectPtr<UMaterialInterface> DecalMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Targeting")
	bool bConfirmOnPrimaryClick = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Targeting")
	bool bCancelOnSecondaryClick = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Targeting")
	bool bCancelOnEscape = true;

private:
	bool UpdateTargetHit();
	void UpdateDecal();
	bool IsTargetInRange() const;

	TWeakObjectPtr<APlayerController> CachedPlayerController;
	TWeakObjectPtr<AActor> CachedSourceActor;
	FHitResult CurrentHit;
	bool bHasCurrentHit = false;
};
