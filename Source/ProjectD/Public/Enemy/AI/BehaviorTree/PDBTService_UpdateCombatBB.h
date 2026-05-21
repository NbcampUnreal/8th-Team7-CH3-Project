#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "PDBTService_UpdateCombatBB.generated.h"

/**
 * 주기적으로 CombatComponent 상태(CanAttack, HasNoiseHint 만료) → Blackboard 갱신.
 * BT Decorator/Selector 가 실시간 신호를 보고 분기하기 위함.
 *
 * 추가 책임 — Stale Target 강제 해제:
 *  - Squad 통보(NotifyAlliesInRadius) 로 들어온 타겟은 perception 의 Lost 가 안 와서 영구 추적됨.
 *  - 본 service 가 매 tick 거리/생존을 검증해 무효 타겟이면 BB/Combat 양쪽 clear.
 *  - 해제 시 LastSeenLocation 은 마지막 위치로 갱신해 BT 가 investigate 모드로 자연 전이.
 */
UCLASS(meta = (DisplayName = "PD Update Combat BB"))
class PROJECTD_API UPDBTService_UpdateCombatBB : public UBTService
{
	GENERATED_BODY()

public:
	UPDBTService_UpdateCombatBB();

	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

protected:
	/**
	 * 타겟 강제 해제 거리(cm). 이 거리를 넘은 TargetActor 는 perception 신호와 무관하게 BB/Combat 모두에서 clear.
	 * 0 이하 → AIController 의 PDPerceptionComponent.LoseSightRadius 를 자동 사용 (perception 과 동일 정책).
	 */
	UPROPERTY(EditAnywhere, Category = "PD|Combat|StaleTarget", meta = (ClampMin = "0.0"))
	float MaxTrackDistance = 0.f;

	/** 해제 거리에 곱해질 안전 마진. perception 의 Lost 가 잠깐 늦게 와도 service 가 먼저 끊지 않도록 살짝 키워둠. */
	UPROPERTY(EditAnywhere, Category = "PD|Combat|StaleTarget", meta = (ClampMin = "1.0", ClampMax = "3.0"))
	float MaxTrackDistanceMargin = 1.1f;
};
