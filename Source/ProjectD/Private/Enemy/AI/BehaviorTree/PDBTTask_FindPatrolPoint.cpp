#include "Enemy/AI/BehaviorTree/PDBTTask_FindPatrolPoint.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Float.h"
#include "Enemy/AI/BehaviorTree/PDBTKeys.h"
#include "Enemy/AI/Controllers/PDEnemyAIControllerBase.h"
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

	FVector Origin;
	float   Radius;
	if (bWander)
	{
		// 자유 배회: Home 무시, 현재 위치 기준 WanderRadius 로 즉시 새 점.
		Origin = Pawn->GetActorLocation();
		Radius = WanderRadius;
	}
	else
	{
		const FVector HomeLoc = BB->GetValueAsVector(HomeLocation.SelectedKeyName);
		Origin = HomeLoc.IsNearlyZero() ? Pawn->GetActorLocation() : HomeLoc;

		Radius = BB->GetValueAsFloat(PatrolRadius.SelectedKeyName);
		if (Radius <= 0.f) Radius = DefaultPatrolRadius;
	}

	FNavLocation Result;
	if (!NavSys->GetRandomReachablePointInRadius(Origin, Radius, Result))
	{
		return EBTNodeResult::Failed;
	}

	const FName KeyName = OutPatrolPoint.SelectedKeyName;
	if (BB->GetKeyID(KeyName) == FBlackboard::InvalidKey)
	{
		UE_LOG(LogPDAI, Warning,
			TEXT("[%s] FindPatrolPoint FAIL — Blackboard 자산에 Vector 키 '%s' 없음."),
			*Pawn->GetName(), *KeyName.ToString());
		return EBTNodeResult::Failed;
	}

	BB->SetValueAsVector(KeyName, Result.Location);
	return EBTNodeResult::Succeeded;
}
