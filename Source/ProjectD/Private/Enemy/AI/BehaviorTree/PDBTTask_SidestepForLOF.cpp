#include "Enemy/AI/BehaviorTree/PDBTTask_SidestepForLOF.h"

#include "AIController.h"
#include "Enemy/Components/PDCombatComponent.h"
#include "GameFramework/Pawn.h"
#include "Navigation/PathFollowingComponent.h"

UPDBTTask_SidestepForLOF::UPDBTTask_SidestepForLOF()
{
	NodeName = TEXT("PD Sidestep For LOF");
	bNotifyTick = true;
	bNotifyTaskFinished = true;
}

uint16 UPDBTTask_SidestepForLOF::GetInstanceMemorySize() const
{
	return sizeof(FPDSidestepLOFMemory);
}

namespace
{
	bool HasArrivedSidestep(AAIController* AI, const FVector& Target, float Acceptance)
	{
		if (!AI) return false;
		APawn* P = AI->GetPawn();
		if (!P) return false;
		const float DistSq = FVector::DistSquared2D(P->GetActorLocation(), Target);
		return DistSq <= Acceptance * Acceptance;
	}
}

EBTNodeResult::Type UPDBTTask_SidestepForLOF::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AI = OwnerComp.GetAIOwner();
	APawn* Pawn = AI ? AI->GetPawn() : nullptr;
	UPDCombatComponent* Combat = Pawn ? Pawn->FindComponentByClass<UPDCombatComponent>() : nullptr;
	if (!AI || !Pawn || !Combat || !Combat->HasValidTarget()) return EBTNodeResult::Failed;

	AActor* Target = Combat->GetCurrentTarget();
	if (!Target) return EBTNodeResult::Failed;

	// 적 대치선 기준 우측 = -Cross(ToTarget, Up) (UE 좌수 좌표계).
	const FVector ToTarget = (Target->GetActorLocation() - Pawn->GetActorLocation()) * FVector(1, 1, 0);
	const FVector ToTargetDir = ToTarget.GetSafeNormal();
	if (ToTargetDir.IsNearlyZero()) return EBTNodeResult::Failed;

	const FVector RightDir = -FVector::CrossProduct(ToTargetDir, FVector::UpVector);
	const FVector StepDir  = bGoRight ? RightDir : -RightDir;

	FPDSidestepLOFMemory* Mem = reinterpret_cast<FPDSidestepLOFMemory*>(NodeMemory);
	Mem->TargetSpot = Pawn->GetActorLocation() + StepDir * SidestepDistance;
	Mem->Elapsed = 0.f;

	// sidestep 중 적 응시 유지 — 회전이 이동을 따라가지 않도록.
	AI->SetFocus(Target, EAIFocusPriority::Gameplay);

	const EPathFollowingRequestResult::Type MoveResult =
		AI->MoveToLocation(Mem->TargetSpot, MoveAcceptanceRadius, /*bStopOnOverlap=*/true,
			/*bUsePathfinding=*/true, /*bProjectDestinationToNavigation=*/true,
			/*bCanStrafe=*/true);

	if (MoveResult == EPathFollowingRequestResult::Failed)
	{
		return EBTNodeResult::Failed;
	}

	// 이미 도착 — 즉시 성공 처리(MoveTo 가 AlreadyAtGoal 일 때).
	if (MoveResult == EPathFollowingRequestResult::AlreadyAtGoal)
	{
		return EBTNodeResult::Succeeded;
	}

	return EBTNodeResult::InProgress;
}

void UPDBTTask_SidestepForLOF::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AI = OwnerComp.GetAIOwner();
	APawn* Pawn = AI ? AI->GetPawn() : nullptr;
	UPDCombatComponent* Combat = Pawn ? Pawn->FindComponentByClass<UPDCombatComponent>() : nullptr;
	if (!AI || !Pawn || !Combat)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	FPDSidestepLOFMemory* Mem = reinterpret_cast<FPDSidestepLOFMemory*>(NodeMemory);
	Mem->Elapsed += DeltaSeconds;

	// 안전 timeout.
	if (Mem->Elapsed >= MaxDuration)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	// 도착했으면 즉시 종료.
	if (HasArrivedSidestep(AI, Mem->TargetSpot, MoveAcceptanceRadius))
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	// LOF 풀렸으면 더 이동할 필요 없음 — 단, MinHold 지나야 (떨림 방지).
	if (Mem->Elapsed >= MinHoldDuration && !Combat->IsFriendlyInLineOfFire())
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}

EBTNodeResult::Type UPDBTTask_SidestepForLOF::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* /*NodeMemory*/)
{
	Super::AbortTask(OwnerComp, nullptr);
	return EBTNodeResult::Aborted;
}

void UPDBTTask_SidestepForLOF::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);

	if (AAIController* AI = OwnerComp.GetAIOwner())
	{
		AI->ClearFocus(EAIFocusPriority::Gameplay);
		AI->StopMovement();
	}
}
