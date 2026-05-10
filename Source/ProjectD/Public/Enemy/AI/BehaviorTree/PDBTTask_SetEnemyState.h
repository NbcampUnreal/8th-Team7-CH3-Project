#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "Enemy/Types/EnemyTypes.h"
#include "PDBTTask_SetEnemyState.generated.h"

/**
 * Pawn(APDEnemyBase)->SetEnemyState 호출 + Blackboard EnemyState 동기화.
 * 상태별 hook(OnEnterState_*) 와 BlueprintImplementableEvent(OnEnemyStateChanged) 가
 * 동시에 발화되도록 한 곳에서 호출.
 */
UCLASS(meta = (DisplayName = "PD Set Enemy State"))
class PROJECTD_API UPDBTTask_SetEnemyState : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UPDBTTask_SetEnemyState();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:
	UPROPERTY(EditAnywhere, Category = "PD|State")
	EPDEnemyState NewState = EPDEnemyState::Idle;

	UPROPERTY(EditAnywhere, Category = "PD|State")
	struct FBlackboardKeySelector EnemyStateKey;
};
