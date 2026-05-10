#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Enemy/Types/EnemyTypes.h"
#include "PDStatComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPDOnStaminaChanged, float, NewValue, float, MaxValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPDOnStaminaStatusChanged, EPDStaminaStatus, OldStatus, EPDStaminaStatus, NewStatus);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPDOnStaminaDepleted);

/**
 * 적 전용 Stamina 컴포넌트.
 * HP는 GAS PDAttributeSet이 단일 진실 원천이므로 본 컴포넌트는 손대지 않음.
 * Stamina를 컴포넌트로 분리한 이유: AttributeSet 비종속 + Biped 한정 사용.
 */
UCLASS(ClassGroup = (PD), meta = (BlueprintSpawnableComponent))
class PROJECTD_API UPDStatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPDStatComponent();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "PD|Stat|Stamina")
	void SetUseStamina(bool bInUseStamina);

	UFUNCTION(BlueprintPure, Category = "PD|Stat|Stamina")
	FORCEINLINE bool IsUsingStamina() const { return bUseStamina; }

	UFUNCTION(BlueprintPure, Category = "PD|Stat|Stamina")
	float GetCurrentStamina() const;

	UFUNCTION(BlueprintPure, Category = "PD|Stat|Stamina")
	float GetMaxStamina() const;

	UFUNCTION(BlueprintPure, Category = "PD|Stat|Stamina")
	float GetStaminaPercent() const;

	UFUNCTION(BlueprintPure, Category = "PD|Stat|Stamina")
	EPDStaminaStatus GetStaminaStatus() const;

	UFUNCTION(BlueprintCallable, Category = "PD|Stat|Stamina")
	void SetStamina(float NewValue);

	UFUNCTION(BlueprintCallable, Category = "PD|Stat|Stamina")
	void SetMaxStamina(float NewMax);

	/** 양수만큼 소비. true=요청량 전부 소비 가능. */
	UFUNCTION(BlueprintCallable, Category = "PD|Stat|Stamina")
	bool ConsumeStamina(float Amount);

	UFUNCTION(BlueprintCallable, Category = "PD|Stat|Stamina")
	void RecoverStamina(float Amount);

	UPROPERTY(BlueprintAssignable, Category = "PD|Stat|Stamina")
	FPDOnStaminaChanged OnStaminaChanged;

	UPROPERTY(BlueprintAssignable, Category = "PD|Stat|Stamina")
	FPDOnStaminaStatusChanged OnStaminaStatusChanged;

	UPROPERTY(BlueprintAssignable, Category = "PD|Stat|Stamina")
	FPDOnStaminaDepleted OnStaminaDepleted;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Stat|Stamina")
	bool bUseStamina = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Stat|Stamina", meta = (ClampMin = "0.0"))
	float MaxStamina = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Stat|Stamina", meta = (ClampMin = "0.0"))
	float InitialStamina = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Stat|Stamina", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FullThreshold = 0.8f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Stat|Stamina", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float OptimalThreshold = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Stat|Stamina", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LowThreshold = 0.2f;

private:
	EPDStaminaStatus ComputeStaminaStatus(float Percent) const;
	void ApplyStaminaChange_Internal(float NewValue);

	EPDStaminaStatus CachedStaminaStatus = EPDStaminaStatus::None;
	float CurrentStamina = 0.f;

	mutable FCriticalSection StaminaCS;
};
