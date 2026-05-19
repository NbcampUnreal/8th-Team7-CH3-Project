#include "Enemy/AI/BehaviorTree/PDBTTask_GetNextWaypoint.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Enemy/AI/Controllers/PDEnemyAIControllerBase.h"
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
	if (!Biped->GetNextPatrolWaypoint(NextLoc))
	{
		UE_LOG(LogPDAI, Warning, TEXT("[%s] GetNextWaypoint FAIL — GetNextPatrolWaypoint returned false (bUsePatrolRoute off or points<2)"),
			*Biped->GetName());
		return EBTNodeResult::Failed;
	}

	// BB 키 존재 검증 — 자산에 키가 없거나 이름이 다르면 SetValueAsVector 가 silent no-op.
	// 한 번이라도 set 이 실패하면 Patrol 분기가 영원히 (0,0,0) 으로 MoveTo 시도 → BT 정지처럼 보임.
	const FName KeyName = OutPatrolPoint.SelectedKeyName;
	const FBlackboard::FKey KeyID = BB->GetKeyID(KeyName);
	if (KeyID == FBlackboard::InvalidKey)
	{
		UE_LOG(LogPDAI, Warning,
			TEXT("[%s] GetNextWaypoint FAIL — Blackboard asset has no Vector key named '%s'. "
			     "BB 자산에 Vector 키 추가하거나 BT 노드의 OutPatrolPoint 드롭다운을 올바른 키로 지정."),
			*Biped->GetName(), *KeyName.ToString());
		return EBTNodeResult::Failed;
	}

	BB->SetValueAsVector(KeyName, NextLoc);

	UE_LOG(LogPDAI, Verbose, TEXT("[%s] GetNextWaypoint SET — Key='%s' Loc=(%.0f, %.0f, %.0f)"),
		*Biped->GetName(), *KeyName.ToString(), NextLoc.X, NextLoc.Y, NextLoc.Z);

	return EBTNodeResult::Succeeded;
}
