#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "PDBTTask_FireAtTarget.generated.h"

/**
 * 매 Tick CombatComponent->RequestAttack() 시도.
 * 타겟이 사라지면 Failed 반환 → 부모 Selector 가 다른 분기 평가.
 * Focus 는 EnterTask 에서 한 번만 설정, AbortTask/Finished 에서 해제.
 */
UCLASS(meta = (DisplayName = "PD Fire At Target"))
class PROJECTD_API UPDBTTask_FireAtTarget : public UBTTask_BlackboardBase
{
	GENERATED_BODY()

public:
	UPDBTTask_FireAtTarget();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;

protected:
	/** SetFocus 사용 여부. 디자이너가 끄면 Pawn 회전 제어를 BT 외부 로직에 위임. */
	UPROPERTY(EditAnywhere, Category = "PD|Fire")
	bool bSetFocusOnTarget = true;
};
