#pragma once

#include "CoreMinimal.h"
#include "Characters/Base/PDEnemyBase.h"
#include "PDBipedEnemy.generated.h"

class UPDCombatComponent;
class USplineComponent;

/**
 * 이족 보행 적 추상 베이스.
 *  - Stamina 는 PDCharacterBase 의 GAS UPDAttributeSet (Stamina/MaxStamina) 을 단일 진실 원천으로 사용.
 *  - GetStaminaStatus 를 AttributeSet 퍼센트 + 본 클래스 임계값으로 분류.
 *  - CombatComponent 의무 보유.
 *
 * "Biped" 카테고리를 클래스 계층으로 명시 → 4족/비행 적이 추가될 때
 * Stamina/이족 가정이 새지 않도록 격리.
 */
UCLASS(Abstract, Blueprintable)
class PROJECTD_API APDBipedEnemy : public APDEnemyBase
{
	GENERATED_BODY()

public:
	APDBipedEnemy();

	virtual EPDStaminaStatus GetStaminaStatus_Implementation() const override;

	UFUNCTION(BlueprintPure, Category = "PD|Combat")
	FORCEINLINE UPDCombatComponent* GetCombatComponent() const { return CombatComponent; }

	UFUNCTION(BlueprintPure, Category = "PD|Patrol")
	FORCEINLINE USplineComponent* GetPatrolRouteSpline() const { return PatrolRouteSpline; }

	UFUNCTION(BlueprintPure, Category = "PD|Patrol")
	FORCEINLINE bool HasPatrolRoute() const { return bUsePatrolRoute && CachedPatrolWaypoints.Num() >= 2; }

	/** 다음 patrol 좌표 산출 + 내부 인덱스 진행. 결과는 월드 좌표. 반환 false=경로 미설정. */
	UFUNCTION(BlueprintCallable, Category = "PD|Patrol")
	bool GetNextPatrolWaypoint(FVector& OutLocation);

protected:
	virtual void BeginPlay() override;
	/** Stamina 퍼센트 임계값 — 이상이면 Full. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Stat|Stamina",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float StaminaFullThreshold = 0.8f;

	/** Stamina 퍼센트 임계값 — 이상이면 Optimal. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Stat|Stamina",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float StaminaOptimalThreshold = 0.5f;

	/** Stamina 퍼센트 임계값 — 이상이면 Low, 미만이면 Exhausted. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Stat|Stamina",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float StaminaLowThreshold = 0.2f;

	/** Patrol 경로 사용 여부. false 면 BT 가 Patrol 분기에서 랜덤 샘플(FindPatrolPoint) 로 폴백. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Patrol")
	bool bUsePatrolRoute = false;

	/** true=마지막 점 도달 후 1번 점으로 즉시 점프 (loop). false=역방향으로 되돌아가기 (ping-pong). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Patrol")
	bool bLoopPatrolRoute = true;

private:
	UPROPERTY(VisibleAnywhere, Category = "PD|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPDCombatComponent> CombatComponent;

	/** 디자이너가 viewport 에서 핸들 드래그로 점 위치 조정 — 선택 시 spline 라인이 보임. 점 추가/삭제는 우클릭 메뉴. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Patrol", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USplineComponent> PatrolRouteSpline;

	/** BeginPlay 시점에 spline 의 월드 좌표를 캐싱 — runtime 에 적이 이동해도 patrol 점은 고정. */
	TArray<FVector> CachedPatrolWaypoints;

	int32 CurrentPatrolIndex = 0;

	/** ping-pong 모드용 방향 (+1=정방향, -1=역방향). loop 모드에선 무시. */
	int32 PatrolStepDir = 1;
};
