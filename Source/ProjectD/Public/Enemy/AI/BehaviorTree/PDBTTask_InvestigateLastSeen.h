#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "PDBTTask_InvestigateLastSeen.generated.h"

/**
 * BB.LastSeenLocation 으로 이동 후, BB.LastSeenDirection 방향으로 SetFocalPoint 한 채
 * PatrolDuration 만큼 짧게 둘러보다 Succeeded.
 *
 * 흐름:
 *  1) ExecuteTask: MoveToLocation(LastSeenLocation). 상태 = Moving.
 *  2) TickTask:
 *     - Moving 중 도착 → SetFocalPoint(Pawn->Loc + Direction*GazeDistance), 상태 = Scanning, 타이머 시작.
 *     - Scanning 중 PatrolDuration 경과 → FinishLatentTask(Succeeded).
 *  3) OnTaskFinished: ClearFocus.
 *
 * BT 전이 의도: 본 Task 완료 → BT 가 Patrol(HomeLocation 복귀) 분기로 자연 이동.
 */
UCLASS(meta = (DisplayName = "PD Investigate Last Seen"))
class PROJECTD_API UPDBTTask_InvestigateLastSeen : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UPDBTTask_InvestigateLastSeen();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;
	virtual uint16 GetInstanceMemorySize() const override;

protected:
	/** 도착 후 둘러보는 시간(초). 사용자 명세 3초. */
	UPROPERTY(EditAnywhere, Category = "PD|Investigate", meta = (ClampMin = "0.1"))
	float PatrolDuration = 3.f;

	/** MoveTo AcceptanceRadius(cm). */
	UPROPERTY(EditAnywhere, Category = "PD|Investigate", meta = (ClampMin = "1.0"))
	float MoveAcceptanceRadius = 100.f;

	/** 도착 후 SetFocalPoint 거리(cm). 너무 짧으면 회전 불안정. */
	UPROPERTY(EditAnywhere, Category = "PD|Investigate", meta = (ClampMin = "50.0"))
	float GazeDistance = 500.f;

	/** 이동 단계 safety timeout(초). 막힌 NavMesh 대비. */
	UPROPERTY(EditAnywhere, Category = "PD|Investigate", meta = (ClampMin = "1.0"))
	float MoveTimeout = 8.f;
};

enum class EPDInvestigateState : uint8
{
	Moving,
	Scanning,
};

struct FPDInvestigateMemory
{
	float MoveElapsed = 0.f;
	float ScanElapsed = 0.f;
	FVector Direction = FVector::ForwardVector;
	EPDInvestigateState State = EPDInvestigateState::Moving;
};
