#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "PDBTTask_ConsumeNoiseHint.generated.h"

/**
 * Investigate 종료 시 NoiseHint 를 소비.
 * CombatComponent + Blackboard(HasNoiseHint) 를 같이 클리어 → 같은 위치를 무한 조사하지 않도록.
 */
UCLASS(meta = (DisplayName = "PD Consume Noise Hint"))
class PROJECTD_API UPDBTTask_ConsumeNoiseHint : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UPDBTTask_ConsumeNoiseHint();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
