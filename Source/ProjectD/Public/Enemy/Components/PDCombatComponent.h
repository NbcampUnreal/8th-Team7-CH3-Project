#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PDCombatComponent.generated.h"

class AActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam (FPDOnTargetChanged, AActor*, NewTarget);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam (FPDOnAttackRequested, AActor*, Target);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPDOnNoiseHintChanged, FVector, Location, bool, bHasHint);

/**
 * 적 전투 컴포넌트.
 *  - 현재 시각 타겟(CurrentTarget)과 청각 의심 위치(NoiseHint)를 분리 보관.
 *  - 공격 쿨다운만 관리하고 실제 발사는 OnAttackRequested 구독자(무기/캐릭터)가 수행.
 *  - 같은 팀에 타겟 전파(NotifyAlliesInRadius).
 *
 * 결합 분리: 데미지/이펙트/애니메이션 어느 것도 본 컴포넌트에 의존하지 않음.
 *           추후 GAS Ability 로 교체 시 본 컴포넌트는 디스패처 역할만 유지.
 */
UCLASS(ClassGroup = (PD), meta = (BlueprintSpawnableComponent))
class PROJECTD_API UPDCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPDCombatComponent();

	UFUNCTION(BlueprintCallable, Category = "PD|Combat")
	void SetCurrentTarget(AActor* NewTarget);

	UFUNCTION(BlueprintCallable, Category = "PD|Combat")
	void ClearCurrentTarget();

	UFUNCTION(BlueprintPure, Category = "PD|Combat")
	AActor* GetCurrentTarget() const { return CurrentTarget.Get(); }

	UFUNCTION(BlueprintPure, Category = "PD|Combat")
	bool HasValidTarget() const;

	UFUNCTION(BlueprintPure, Category = "PD|Combat")
	bool CanAttack() const;

	/** 쿨다운/사거리/타겟 검증 후 OnAttackRequested broadcast. true=요청 성공. */
	UFUNCTION(BlueprintCallable, Category = "PD|Combat")
	bool RequestAttack();

	UFUNCTION(BlueprintPure, Category = "PD|Combat")
	bool IsOnCooldown() const;

	UFUNCTION(BlueprintPure, Category = "PD|Combat")
	float GetCooldownRemaining() const;

	UFUNCTION(BlueprintPure, Category = "PD|Combat")
	FORCEINLINE float GetAttackRange() const { return AttackRange; }

	UFUNCTION(BlueprintCallable, Category = "PD|Combat|Noise")
	void SetLastNoiseLocation(AActor* NoiseInstigator, const FVector& Location);

	UFUNCTION(BlueprintCallable, Category = "PD|Combat|Noise")
	void ClearNoiseHint();

	UFUNCTION(BlueprintPure, Category = "PD|Combat|Noise")
	bool HasNoiseHint() const { return bHasNoiseHint; }

	UFUNCTION(BlueprintPure, Category = "PD|Combat|Noise")
	FVector GetLastNoiseLocation() const { return LastNoiseLocation; }

	UFUNCTION(BlueprintPure, Category = "PD|Combat|Noise")
	AActor* GetLastNoiseInstigator() const { return LastNoiseInstigator.Get(); }

	UPROPERTY(BlueprintAssignable, Category = "PD|Combat|Noise")
	FPDOnNoiseHintChanged OnNoiseHintChanged;

	/** Radius 내 같은 팀에게 타겟 전파(이미 같은 타겟이면 skip). */
	UFUNCTION(BlueprintCallable, Category = "PD|Combat|Squad")
	void NotifyAlliesInRadius(float Radius, AActor* SharedTarget);

	UPROPERTY(BlueprintAssignable, Category = "PD|Combat")
	FPDOnTargetChanged OnTargetChanged;

	UPROPERTY(BlueprintAssignable, Category = "PD|Combat")
	FPDOnAttackRequested OnAttackRequested;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Combat", meta = (ClampMin = "0.0"))
	float AttackCooldown = 1.0f;

	/** RequestAttack 시 타겟까지의 최대 거리. SightRadius 보다 짧게 두어 추격 페이즈를 확보. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Combat", meta = (ClampMin = "0.0"))
	float AttackRange = 800.f;

private:
	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> CurrentTarget;

	float LastAttackTime = -1.f;

	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> LastNoiseInstigator;

	UPROPERTY(Transient)
	FVector LastNoiseLocation = FVector::ZeroVector;

	UPROPERTY(Transient)
	bool bHasNoiseHint = false;
};
