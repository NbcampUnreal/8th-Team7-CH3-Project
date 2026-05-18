#include "Enemy/AI/BehaviorTree/PDBTTask_GetNextWaypoint.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Enemy/Characters/PDBipedEnemy.h"
#include "GameFramework/Pawn.h"

UPDBTTask_GetNextWaypoint::UPDBTTask_GetNextWaypoint()
{
	NodeName = TEXT("PD Get Next Waypoint");

	OutPatrolPoint.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UPDBTTask_GetNextWaypoint, OutPatrolPoint));
	OutPatrolPoint.SelectedKeyName = TEXT("OutPatrolPoint");
}

void UPDBTTask_GetNextWaypoint::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		OutPatrolPoint.ResolveSelectedKey(*BBAsset);
	}
}

EBTNodeResult::Type UPDBTTask_GetNextWaypoint::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* /*NodeMemory*/)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* Pawn = AIController ? AIController->GetPawn() : nullptr;
	APDBipedEnemy* Biped = Cast<APDBipedEnemy>(Pawn);
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!Biped || !BB) return EBTNodeResult::Failed;

	FVector NextLoc;
	if (!Biped->GetNextPatrolWaypoint(NextLoc)) return EBTNodeResult::Failed;

	BB->SetValueAsVector(OutPatrolPoint.SelectedKeyName, NextLoc);
	return EBTNodeResult::Succeeded;
}
