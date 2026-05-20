#include "Enemy/AI/BehaviorTree/PDBTTask_ReleaseCover.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Enemy/AI/BehaviorTree/PDBTKeys.h"
#include "Enemy/Characters/PDEliteSoldier.h"

UPDBTTask_ReleaseCover::UPDBTTask_ReleaseCover()
{
	NodeName = TEXT("PD Release Cover");
}

EBTNodeResult::Type UPDBTTask_ReleaseCover::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AI = OwnerComp.GetAIOwner();
	APawn* Pawn = AI ? AI->GetPawn() : nullptr;
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();

	if (APDEliteSoldier* Elite = Cast<APDEliteSoldier>(Pawn))
	{
		Elite->SetInCover(false);
	}

	if (BB)
	{
		BB->ClearValue(PDBTKeys::CoverActor);
		BB->SetValueAsBool(PDBTKeys::bHasCoverTarget, false);
	}

	return EBTNodeResult::Succeeded;
}
