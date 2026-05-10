#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "PDBTTask_ClearBBValue.generated.h"

/**
 * 지정한 Blackboard 키를 Clear.
 * UE5 BT 에 내장 "Clear Blackboard Value" Task 가 없어 추가한 범용 유틸.
 * Vector/Object/Bool 등 키 종류와 무관하게 동작.
 */
UCLASS(meta = (DisplayName = "PD Clear BB Value"))
class PROJECTD_API UPDBTTask_ClearBBValue : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UPDBTTask_ClearBBValue();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;

protected:
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;

	/** Clear 대상 BB 키. */
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetKey;
};
