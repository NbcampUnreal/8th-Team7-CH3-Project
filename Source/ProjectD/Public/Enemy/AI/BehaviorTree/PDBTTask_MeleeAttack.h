#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "PDBTTask_MeleeAttack.generated.h"

/**
 * 근접 휘두름 1회 task — FireAtTarget 의 근접 전용 대체.
 *  - ExecuteTask: 타겟 방향으로 Snap rotate 후 Combat->RequestAttack() 1회 호출.
 *    실패(거리/쿨다운/타겟 무효)면 Failed → BT 부모가 Chase 등 다른 분기로 즉시 복귀.
 *  - Tick: AttackDuration 만큼 점유 → 동일 task 가 휘두름 도중 재진입하지 않음.
 *  - 거리/쿨다운 게이트는 BT 데코레이터(IsTargetInRange) 와 CombatComponent 가 이중으로 담당.
 *
 * 디자이너 사용 흐름:
 *  [Combat 분기] Selector
 *   ├─ Decorator: IsTargetInRange == true (Lower Priority Abort)
 *   ├─ Decorator: HasLOSToTarget   == true
 *   └─ Sequence
 *       ├─ SetEnemyState(Combat)
 *       └─ PDBTTask_MeleeAttack
 */
UCLASS(meta = (DisplayName = "PD Melee Attack"))
class PROJECTD_API UPDBTTask_MeleeAttack : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UPDBTTask_MeleeAttack();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual uint16 GetInstanceMemorySize() const override;

protected:
	/** 휘두름 1회 task 점유 시간(초). 몽타주 길이 + 약간의 여유. AttackCooldown 보다 크거나 같게 두는 것을 권장. */
	UPROPERTY(EditAnywhere, Category = "PD|Melee", meta = (ClampMin = "0.1"))
	float AttackDuration = 1.2f;

	/** true 면 ExecuteTask 에서 즉시 정렬. false 면 회전은 외부(RotateToFaceTarget)에 위임. */
	UPROPERTY(EditAnywhere, Category = "PD|Melee")
	bool bSnapRotateToTarget = true;

	/** 휘두름 도중 타겟 무효(사망/소실)이면 Abort 처리. false 면 모션 끝까지 점유. */
	UPROPERTY(EditAnywhere, Category = "PD|Melee")
	bool bAbortIfTargetLost = true;
};
