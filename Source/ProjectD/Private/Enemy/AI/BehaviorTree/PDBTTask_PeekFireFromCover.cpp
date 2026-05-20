#include "Enemy/AI/BehaviorTree/PDBTTask_PeekFireFromCover.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Enemy/AI/BehaviorTree/PDBTKeys.h"
#include "Enemy/Characters/PDEliteSoldier.h"
#include "GameFramework/Pawn.h"
#include "Navigation/PathFollowingComponent.h"

UPDBTTask_PeekFireFromCover::UPDBTTask_PeekFireFromCover()
{
	NodeName = TEXT("PD Peek Fire From Cover (Right)");
	BlackboardKey.SelectedKeyName = PDBTKeys::TargetActor;

	bNotifyTick = true;
	bNotifyTaskFinished = true;
}

uint16 UPDBTTask_PeekFireFromCover::GetInstanceMemorySize() const
{
	return sizeof(FPDPeekFireTaskMemory);
}

namespace
{
	bool HasArrived(AAIController* AI, const FVector& Target, float AcceptanceRadius)
	{
		if (!AI) return false;
		APawn* P = AI->GetPawn();
		if (!P) return false;
		const float DistSq = FVector::DistSquared2D(P->GetActorLocation(), Target);
		return DistSq <= AcceptanceRadius * AcceptanceRadius;
	}
}

EBTNodeResult::Type UPDBTTask_PeekFireFromCover::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	AAIController* AI = OwnerComp.GetAIOwner();
	APawn* Pawn = AI ? AI->GetPawn() : nullptr;
	AActor* Target = BB ? Cast<AActor>(BB->GetValueAsObject(BlackboardKey.SelectedKeyName)) : nullptr;

	if (!Pawn || !Target) return EBTNodeResult::Failed;

	FPDPeekFireTaskMemory* Mem = reinterpret_cast<FPDPeekFireTaskMemory*>(NodeMemory);
	Mem->OriginalLoc  = Pawn->GetActorLocation();
	Mem->TotalElapsed = 0.f;
	Mem->FireElapsed  = 0.f;
	Mem->State        = EPDPeekFireState::Peeking;

	// 적 방향 기준 우측 = Cross(ToTarget, Up). Cross 결과 부호는 좌표계에 따라 다르므로
	// 본 엔진(좌수, Z up)에서 ToTarget × Up 은 왼쪽 — 우측은 그 반대(-1 곱).
	const FVector ToTarget = (Target->GetActorLocation() - Mem->OriginalLoc) * FVector(1, 1, 0);
	const FVector ToTargetDir = ToTarget.GetSafeNormal();
	if (ToTargetDir.IsNearlyZero()) return EBTNodeResult::Failed;

	const FVector RightDir = -FVector::CrossProduct(ToTargetDir, FVector::UpVector);
	Mem->PeekSpot = Mem->OriginalLoc + RightDir * PeekOffset;

	AI->SetFocus(Target, EAIFocusPriority::Gameplay);

	const EPathFollowingRequestResult::Type MoveResult =
		AI->MoveToLocation(Mem->PeekSpot, MoveAcceptanceRadius, /*bStopOnOverlap=*/true,
			/*bUsePathfinding=*/true, /*bProjectDestinationToNavigation=*/true,
			/*bCanStrafe=*/true);

	if (MoveResult == EPathFollowingRequestResult::Failed)
	{
		return EBTNodeResult::Failed;
	}

	return EBTNodeResult::InProgress;
}

void UPDBTTask_PeekFireFromCover::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AI = OwnerComp.GetAIOwner();
	APawn* Pawn = AI ? AI->GetPawn() : nullptr;
	APDEliteSoldier* Elite = Cast<APDEliteSoldier>(Pawn);

	if (!AI || !Pawn || !Elite)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	FPDPeekFireTaskMemory* Mem = reinterpret_cast<FPDPeekFireTaskMemory*>(NodeMemory);
	Mem->TotalElapsed += DeltaSeconds;

	// 전체 safety timeout — 이동 실패/스택 방지.
	if (Mem->TotalElapsed >= MaxTotalDuration)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	switch (Mem->State)
	{
	case EPDPeekFireState::Peeking:
	{
		if (HasArrived(AI, Mem->PeekSpot, MoveAcceptanceRadius))
		{
			Elite->SetPeeking(true);
			Mem->State = EPDPeekFireState::Firing;
			Mem->FireElapsed = 0.f;
		}
		break;
	}
	case EPDPeekFireState::Firing:
	{
		Mem->FireElapsed += DeltaSeconds;
		if (Mem->FireElapsed >= FireDuration)
		{
			Elite->SetPeeking(false);
			AI->MoveToLocation(Mem->OriginalLoc, MoveAcceptanceRadius, /*bStopOnOverlap=*/true,
				/*bUsePathfinding=*/true, /*bProjectDestinationToNavigation=*/true,
				/*bCanStrafe=*/true);
			Mem->State = EPDPeekFireState::Returning;
		}
		break;
	}
	case EPDPeekFireState::Returning:
	{
		if (HasArrived(AI, Mem->OriginalLoc, MoveAcceptanceRadius))
		{
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		}
		break;
	}
	}
}

EBTNodeResult::Type UPDBTTask_PeekFireFromCover::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	// 사격 루프 정지 + 회전 잠금 해제는 OnTaskFinished 에서 일괄 처리.
	Super::AbortTask(OwnerComp, NodeMemory);
	return EBTNodeResult::Aborted;
}

void UPDBTTask_PeekFireFromCover::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);

	AAIController* AI = OwnerComp.GetAIOwner();
	if (!AI) return;

	// 진행 중이던 사격 무조건 정지(피크가 외부 abort 로 끊겨도 발사 루프 잔존 방지).
	if (APDEliteSoldier* Elite = Cast<APDEliteSoldier>(AI->GetPawn()))
	{
		Elite->SetPeeking(false);
	}

	AI->ClearFocus(EAIFocusPriority::Gameplay);

	// 강제 종료 시 이동 명령도 정리.
	AI->StopMovement();
}
