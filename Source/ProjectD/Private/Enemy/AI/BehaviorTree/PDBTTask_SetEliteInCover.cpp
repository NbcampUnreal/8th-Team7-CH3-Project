#include "Enemy/AI/BehaviorTree/PDBTTask_SetEliteInCover.h"

#include "AIController.h"
#include "Enemy/Characters/PDEliteSoldier.h"

UPDBTTask_SetEliteInCover::UPDBTTask_SetEliteInCover()
{
	NodeName = TEXT("PD Set Elite InCover");
}

EBTNodeResult::Type UPDBTTask_SetEliteInCover::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* /*NodeMemory*/)
{
	AAIController* AI = OwnerComp.GetAIOwner();
	APDEliteSoldier* Elite = AI ? Cast<APDEliteSoldier>(AI->GetPawn()) : nullptr;
	if (!Elite) return EBTNodeResult::Failed;

	Elite->SetInCover(bEnter);
	return EBTNodeResult::Succeeded;
}
