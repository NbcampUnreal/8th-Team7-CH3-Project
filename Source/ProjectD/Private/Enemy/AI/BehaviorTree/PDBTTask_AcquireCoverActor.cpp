#include "Enemy/AI/BehaviorTree/PDBTTask_AcquireCoverActor.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Cover/PDCoverBase.h"
#include "Enemy/AI/BehaviorTree/PDBTKeys.h"
#include "Enemy/Characters/PDEliteSoldier.h"

UPDBTTask_AcquireCoverActor::UPDBTTask_AcquireCoverActor()
{
	NodeName = TEXT("PD Acquire Cover Actor");
	BlackboardKey.SelectedKeyName = PDBTKeys::CoverActor;
}

EBTNodeResult::Type UPDBTTask_AcquireCoverActor::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	AAIController* AI = OwnerComp.GetAIOwner();
	APawn* Pawn = AI ? AI->GetPawn() : nullptr;
	if (!BB || !Pawn) return EBTNodeResult::Failed;

	APDCoverBase* Cover = Cast<APDCoverBase>(BB->GetValueAsObject(BlackboardKey.SelectedKeyName));
	if (!Cover || !Cover->IsUsable()) return EBTNodeResult::Failed;

	if (!Cover->TryOccupy(Pawn))
	{
		// 다른 동료가 먼저 잡았을 수 있음 — 폴백 쪽으로.
		return EBTNodeResult::Failed;
	}

	BB->SetValueAsVector(PDBTKeys::CoverLocation, Cover->GetSnapLocation(Pawn));
	BB->SetValueAsBool(PDBTKeys::bHasCoverTarget, true);

	if (APDEliteSoldier* Elite = Cast<APDEliteSoldier>(Pawn))
	{
		Elite->SetInCover(true);
	}

	return EBTNodeResult::Succeeded;
}
