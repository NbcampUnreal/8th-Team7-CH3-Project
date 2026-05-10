#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "PDBTService_UpdateCombatBB.generated.h"

/**
 * 주기적으로 CombatComponent 상태(CanAttack, HasNoiseHint 만료) → Blackboard 갱신.
 * BT Decorator/Selector 가 실시간 신호를 보고 분기하기 위함.
 */
UCLASS(meta = (DisplayName = "PD Update Combat BB"))
class PROJECTD_API UPDBTService_UpdateCombatBB : public UBTService
{
	GENERATED_BODY()

public:
	UPDBTService_UpdateCombatBB();

	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};
