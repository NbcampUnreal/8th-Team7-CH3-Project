#include "Enemy/AI/BehaviorTree/PDBTTask_FindPatrolPoint.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Float.h"
#include "Enemy/AI/BehaviorTree/PDBTKeys.h"
#include "NavigationSystem.h"

UPDBTTask_FindPatrolPoint::UPDBTTask_FindPatrolPoint()
{
	NodeName = TEXT("PD Find Patrol Point");

	OutPatrolPoint.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UPDBTTask_FindPatrolPoint, OutPatrolPoint));
	HomeLocation .AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UPDBTTask_FindPatrolPoint, HomeLocation));
	PatrolRadius .AddFloatFilter (this, GET_MEMBER_NAME_CHECKED(UPDBTTask_FindPatrolPoint, PatrolRadius));

	// 디폴트 키 이름 — PDBTKeys 의 표준 이름과 일치.
	OutPatrolPoint.SelectedKeyName = TEXT("OutPatrolPoint");
	HomeLocation .SelectedKeyName = PDBTKeys::HomeLocation;
	PatrolRadius .SelectedKeyName = PDBTKeys::PatrolRadius;
}

EBTNodeResult::Type UPDBTTask_FindPatrolPoint::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* /*NodeMemory*/)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* Pawn = AIController ? AIController->GetPawn() : nullptr;
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!Pawn || !BB) return EBTNodeResult::Failed;

	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!NavSys) return EBTNodeResult::Failed;

	const FVector HomeLoc = BB->GetValueAsVector(HomeLocation.SelectedKeyName);
	const FVector Origin = HomeLoc.IsNearlyZero() ? Pawn->GetActorLocation() : HomeLoc;

	float Radius = BB->GetValueAsFloat(PatrolRadius.SelectedKeyName);
	if (Radius <= 0.f) Radius = DefaultPatrolRadius;

	FNavLocation Result;
	if (!NavSys->GetRandomReachablePointInRadius(Origin, Radius, Result))
	{
		return EBTNodeResult::Failed;
	}

	BB->SetValueAsVector(OutPatrolPoint.SelectedKeyName, Result.Location);
	return EBTNodeResult::Succeeded;
}
