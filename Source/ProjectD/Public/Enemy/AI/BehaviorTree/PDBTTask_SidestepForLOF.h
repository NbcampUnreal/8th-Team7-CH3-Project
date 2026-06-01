#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "PDBTTask_SidestepForLOF.generated.h"

/**
 * 우군이 사선에 끼었을 때 우측(또는 좌측) 측면으로 짧게 이동해 LOF 를 푸는 보조 Task.
 *
 * Soldier 처럼 cover 없이 서서 쏘는 적이 우군이 발사 사선을 막을 때 쓰는 단순 sidestep.
 *  - 적 응시(SetFocus)는 유지 — sidestep 중에도 회전은 적을 향함.
 *  - 종료 조건 (셋 중 먼저 도달):
 *      1) SidestepDistance 만큼 이동 도착
 *      2) MinHoldDuration 경과 후 LOF 가 풀림(IsFriendlyInLineOfFire==false)
 *      3) MaxDuration 안전 timeout
 *
 * BT 사용 패턴 (Selector 안 우군 사선 분기):
 *  Combat ─ Selector
 *           ├─ Sequence (Decorator: bFriendlyInLineOfFire == True, FlowAbort=Both)
 *           │   └─ PDBTTask_SidestepForLOF
 *           └─ PDBTTask_FireAtTarget
 *
 *  LOF set 되면 Sidestep 실행 → LOF 풀리는 순간 Decorator 가 자기/하위 abort → FireAtTarget 으로 전이.
 */
UCLASS(meta = (DisplayName = "PD Sidestep For LOF"))
class PROJECTD_API UPDBTTask_SidestepForLOF : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UPDBTTask_SidestepForLOF();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;
	virtual uint16 GetInstanceMemorySize() const override;

protected:
	/** 우측으로 이동할 거리(cm). 단발 sidestep 이므로 짧게. */
	UPROPERTY(EditAnywhere, Category = "PD|Sidestep", meta = (ClampMin = "30.0"))
	float SidestepDistance = 150.f;

	/** 시작 직후 이 시간까지는 LOF 풀려도 종료 안 함 — sidestep 시동 확보 + 떨림 방지. */
	UPROPERTY(EditAnywhere, Category = "PD|Sidestep", meta = (ClampMin = "0.0"))
	float MinHoldDuration = 0.3f;

	/** 전체 안전 timeout(초). 이동 막힘 등 대비. */
	UPROPERTY(EditAnywhere, Category = "PD|Sidestep", meta = (ClampMin = "0.5"))
	float MaxDuration = 3.f;

	/** Move 도착 판정 AcceptanceRadius(cm). */
	UPROPERTY(EditAnywhere, Category = "PD|Sidestep", meta = (ClampMin = "1.0"))
	float MoveAcceptanceRadius = 30.f;

	/** true=우측(적 기준 우측). false=좌측. 단발이지만 디자이너 튜닝 여지. */
	UPROPERTY(EditAnywhere, Category = "PD|Sidestep")
	bool bGoRight = true;
};

struct FPDSidestepLOFMemory
{
	FVector TargetSpot = FVector::ZeroVector;
	float Elapsed = 0.f;
};
