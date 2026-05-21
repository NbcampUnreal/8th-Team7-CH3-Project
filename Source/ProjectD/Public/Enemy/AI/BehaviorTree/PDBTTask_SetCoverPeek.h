#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "PDBTTask_SetCoverPeek.generated.h"

/**
 * APDEliteSoldier.SetPeeking(bPeek) 호출 — 발사 루프 on/off + BP 애니메이션 훅.
 * 디자이너는 BT 의 [Peek=true → Wait(PeekDuration) → Peek=false → Wait(HideDuration)] 시퀀스로 리듬 구성.
 */
UCLASS(meta = (DisplayName = "PD Set Cover Peek"))
class PROJECTD_API UPDBTTask_SetCoverPeek : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UPDBTTask_SetCoverPeek();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:
	UPROPERTY(EditAnywhere, Category = "PD|Cover")
	bool bPeek = true;
};
