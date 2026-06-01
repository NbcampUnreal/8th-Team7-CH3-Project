#include "Enemy/AI/BehaviorTree/PDBTTask_InvestigateLastSeen.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Enemy/AI/BehaviorTree/PDBTKeys.h"
#include "GameFramework/Pawn.h"
#include "Navigation/PathFollowingComponent.h"

UPDBTTask_InvestigateLastSeen::UPDBTTask_InvestigateLastSeen()
{
	NodeName = TEXT("PD Investigate Last Seen");
	bNotifyTick = true;
	bNotifyTaskFinished = true;
}

uint16 UPDBTTask_InvestigateLastSeen::GetInstanceMemorySize() const
{
	return sizeof(FPDInvestigateMemory);
}

EBTNodeResult::Type UPDBTTask_InvestigateLastSeen::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	AAIController* AI = OwnerComp.GetAIOwner();
	APawn* Pawn = AI ? AI->GetPawn() : nullptr;
	if (!BB || !Pawn) return EBTNodeResult::Failed;

	const FVector LastSeen = BB->GetValueAsVector(PDBTKeys::LastSeenLocation);
	if (LastSeen.IsNearlyZero()) return EBTNodeResult::Failed;

	FPDInvestigateMemory* Mem = reinterpret_cast<FPDInvestigateMemory*>(NodeMemory);
	Mem->MoveElapsed = 0.f;
	Mem->ScanElapsed = 0.f;
	Mem->State = EPDInvestigateState::Moving;

	// 방향이 null/zero 면 폰 forward 폴백 (그냥 그 자리 응시).
	const FVector Dir = BB->GetValueAsVector(PDBTKeys::LastSeenDirection);
	Mem->Direction = Dir.IsNearlyZero() ? Pawn->GetActorForwardVector() : Dir.GetSafeNormal2D();

	const EPathFollowingRequestResult::Type MoveResult =
		AI->MoveToLocation(LastSeen, MoveAcceptanceRadius, /*bStopOnOverlap=*/true,
			/*bUsePathfinding=*/true, /*bProjectDestinationToNavigation=*/true,
			/*bCanStrafe=*/false);

	if (MoveResult == EPathFollowingRequestResult::Failed) return EBTNodeResult::Failed;

	// 이미 도착해있으면 바로 Scanning 으로.
	if (MoveResult == EPathFollowingRequestResult::AlreadyAtGoal)
	{
		AI->SetFocalPoint(Pawn->GetActorLocation() + Mem->Direction * GazeDistance, EAIFocusPriority::Gameplay);
		Mem->State = EPDInvestigateState::Scanning;
	}

	return EBTNodeResult::InProgress;
}

void UPDBTTask_InvestigateLastSeen::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AI = OwnerComp.GetAIOwner();
	APawn* Pawn = AI ? AI->GetPawn() : nullptr;
	if (!AI || !Pawn)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	FPDInvestigateMemory* Mem = reinterpret_cast<FPDInvestigateMemory*>(NodeMemory);

	switch (Mem->State)
	{
	case EPDInvestigateState::Moving:
	{
		Mem->MoveElapsed += DeltaSeconds;

		// PathFollowing 의 도착 통보 대신 직접 거리 폴링 — Task 간 일관 패턴(PeekFire 와 동일).
		const FVector Goal = OwnerComp.GetBlackboardComponent()->GetValueAsVector(PDBTKeys::LastSeenLocation);
		const float DistSq = FVector::DistSquared2D(Pawn->GetActorLocation(), Goal);
		if (DistSq <= MoveAcceptanceRadius * MoveAcceptanceRadius)
		{
			AI->StopMovement();
			AI->SetFocalPoint(Pawn->GetActorLocation() + Mem->Direction * GazeDistance, EAIFocusPriority::Gameplay);
			Mem->State = EPDInvestigateState::Scanning;
			break;
		}

		if (Mem->MoveElapsed >= MoveTimeout)
		{
			// 막힌 길 — 이동 포기하고 그 자리에서 둘러보고 종료. Patrol(Home) 로 fallback.
			AI->StopMovement();
			AI->SetFocalPoint(Pawn->GetActorLocation() + Mem->Direction * GazeDistance, EAIFocusPriority::Gameplay);
			Mem->State = EPDInvestigateState::Scanning;
		}
		break;
	}
	case EPDInvestigateState::Scanning:
	{
		Mem->ScanElapsed += DeltaSeconds;
		if (Mem->ScanElapsed >= PatrolDuration)
		{
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		}
		break;
	}
	}
}

EBTNodeResult::Type UPDBTTask_InvestigateLastSeen::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::AbortTask(OwnerComp, NodeMemory);
	return EBTNodeResult::Aborted;
}

void UPDBTTask_InvestigateLastSeen::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);

	if (AAIController* AI = OwnerComp.GetAIOwner())
	{
		AI->ClearFocus(EAIFocusPriority::Gameplay);
		AI->StopMovement();
	}
}
