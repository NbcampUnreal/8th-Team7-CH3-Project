#include "Enemy/AI/BehaviorTree/PDBTTask_SetCoverPeek.h"

#include "AIController.h"
#include "Enemy/Characters/PDEliteSoldier.h"

UPDBTTask_SetCoverPeek::UPDBTTask_SetCoverPeek()
{
	NodeName = TEXT("PD Set Cover Peek");
}

EBTNodeResult::Type UPDBTTask_SetCoverPeek::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AI = OwnerComp.GetAIOwner();
	APawn* Pawn = AI ? AI->GetPawn() : nullptr;

	APDEliteSoldier* Elite = Cast<APDEliteSoldier>(Pawn);
	if (!Elite) return EBTNodeResult::Failed;

	Elite->SetPeeking(bPeek);
	return EBTNodeResult::Succeeded;
}
