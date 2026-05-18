#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "PDBTTask_FindPatrolPoint.generated.h"

/**
 * NavMesh 위 임의 지점을 찾아 OutPatrolPoint(BB Vector) 에 기록.
 * 후속으로 UBTTask_MoveTo 가 OutPatrolPoint 를 사용해 이동.
 *
 * 동작 모드:
 *  - bWander = false (기본): HomeLocation 주변 PatrolRadius 안에서 샘플링.
 *  - bWander = true        : Pawn 현재 위치 기준 WanderRadius 안에서 샘플링 → home 무관 배회.
 */
UCLASS(meta = (DisplayName = "PD Find Patrol Point"))
class PROJECTD_API UPDBTTask_FindPatrolPoint : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UPDBTTask_FindPatrolPoint();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:
	UPROPERTY(EditAnywhere, Category = "PD|Patrol")
	struct FBlackboardKeySelector OutPatrolPoint;

	UPROPERTY(EditAnywhere, Category = "PD|Patrol")
	struct FBlackboardKeySelector HomeLocation;

	UPROPERTY(EditAnywhere, Category = "PD|Patrol")
	struct FBlackboardKeySelector PatrolRadius;

	/** BB 의 PatrolRadius 가 0/미설정일 때 사용할 기본 반경. */
	UPROPERTY(EditAnywhere, Category = "PD|Patrol", meta = (ClampMin = "0.0"))
	float DefaultPatrolRadius = 600.f;

	/** true 면 HomeLocation/PatrolRadius BB 키 무시하고 현재 위치 기준 WanderRadius 로 샘플링. */
	UPROPERTY(EditAnywhere, Category = "PD|Patrol")
	bool bWander = false;

	/** Wander 모드 반경(cm). Pawn 현재 위치 기준. */
	UPROPERTY(EditAnywhere, Category = "PD|Patrol", meta = (ClampMin = "100.0", EditCondition = "bWander"))
	float WanderRadius = 800.f;
};
