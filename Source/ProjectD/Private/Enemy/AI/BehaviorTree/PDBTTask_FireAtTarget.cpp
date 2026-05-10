#include "Enemy/AI/BehaviorTree/PDBTTask_FireAtTarget.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Enemy/AI/BehaviorTree/PDBTKeys.h"
#include "Enemy/Components/PDCombatComponent.h"

UPDBTTask_FireAtTarget::UPDBTTask_FireAtTarget()
{
	NodeName = TEXT("PD Fire At Target");
	bNotifyTick = true;
	bNotifyTaskFinished = true;

	// UBTTask_BlackboardBase 의 BlackboardKey 는 기본적으로 모든 타입 허용 — Object 필터링은 디자이너가 BB 자산에서.
	BlackboardKey.SelectedKeyName = PDBTKeys::TargetActor;
}

EBTNodeResult::Type UPDBTTask_FireAtTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController) return EBTNodeResult::Failed;

	APawn* Pawn = AIController->GetPawn();
	UPDCombatComponent* Combat = Pawn ? Pawn->FindComponentByClass<UPDCombatComponent>() : nullptr;
	if (!Combat || !Combat->HasValidTarget())
	{
		return EBTNodeResult::Failed;
	}

	if (bSetFocusOnTarget)
	{
		AIController->SetFocus(Combat->GetCurrentTarget());
	}

	return EBTNodeResult::InProgress;
}

void UPDBTTask_FireAtTarget::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* Pawn = AIController ? AIController->GetPawn() : nullptr;
	UPDCombatComponent* Combat = Pawn ? Pawn->FindComponentByClass<UPDCombatComponent>() : nullptr;

	if (!Combat || !Combat->HasValidTarget())
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// 사거리/쿨다운 검증은 CombatComponent 내부. 본 Tick 은 매 프레임 호출돼도 안전.
	Combat->RequestAttack();
}

EBTNodeResult::Type UPDBTTask_FireAtTarget::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	// 실제 Focus 클리어는 OnTaskFinished 가 일괄 처리.
	return EBTNodeResult::Aborted;
}

void UPDBTTask_FireAtTarget::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	if (AAIController* AIController = OwnerComp.GetAIOwner())
	{
		AIController->ClearFocus(EAIFocusPriority::Gameplay);
	}
	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}
