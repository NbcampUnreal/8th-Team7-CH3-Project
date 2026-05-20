#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "PDBTTask_SetEliteInCover.generated.h"

/**
 * APDEliteSoldier::SetInCover(bEnter) 를 호출하는 단발 Task.
 *  - bEnter=true  : MoveTo 로 cover 위치 도착 후 호출. BP_OnEnterCover 훅 실행.
 *  - bEnter=false : 사이클 종료 시 호출. 진행 중이던 피크/사격도 함께 정지(Set 내부에서 SetPeeking(false)).
 *  - 폰이 APDEliteSoldier 가 아니면 Failed.
 */
UCLASS(meta = (DisplayName = "PD Set Elite InCover"))
class PROJECTD_API UPDBTTask_SetEliteInCover : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UPDBTTask_SetEliteInCover();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:
	UPROPERTY(EditAnywhere, Category = "PD|Cover")
	bool bEnter = true;
};
