#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "PDBTTask_GetNextWaypoint.generated.h"

/**
 * Pawn(APDBipedEnemy)의 다음 patrol waypoint 좌표를 BB 의 Vector 키에 기록.
 *  - 인덱스 진행은 Pawn 측이 보유 (BT 가 다른 분기로 빠졌다 돌아와도 인덱스 유지).
 *  - bUsePatrolRoute=false 이거나 점이 2 개 미만이면 Failed → 부모 Selector 가 랜덤 patrol(FindPatrolPoint) 로 폴백.
 */
UCLASS(meta = (DisplayName = "PD Get Next Waypoint"))
class PROJECTD_API UPDBTTask_GetNextWaypoint : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UPDBTTask_GetNextWaypoint();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;

	/** 결과 저장 BB 키 (Vector). 기본 OutPatrolPoint — FindPatrolPoint 와 동일 키로 MoveTo 재사용. */
	UPROPERTY(EditAnywhere, Category = "PD|Patrol")
	FBlackboardKeySelector OutPatrolPoint;
};
