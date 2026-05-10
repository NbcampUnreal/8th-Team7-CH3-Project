#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "PDBTTask_NotifyAllies.generated.h"

/**
 * Combat 진입 시 호출 — Radius 안 같은 팀 적에게 현재 타겟을 전파.
 * 동료를 함께 활성화해 squad 행동 유도.
 */
UCLASS(meta = (DisplayName = "PD Notify Allies"))
class PROJECTD_API UPDBTTask_NotifyAllies : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UPDBTTask_NotifyAllies();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:
	UPROPERTY(EditAnywhere, Category = "PD|Squad", meta = (ClampMin = "0.0"))
	float NotifyRadius = 1500.f;
};
