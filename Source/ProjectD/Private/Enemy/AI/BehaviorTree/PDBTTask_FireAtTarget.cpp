#include "Enemy/AI/BehaviorTree/PDBTTask_FireAtTarget.h"

#include "AIController.h"
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

	AActor* Target = Combat->GetCurrentTarget();
	if (!Target)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// 타겟 방향 Yaw 계산 — 2D 만 사용 (Pitch 는 애님 aim offset 담당).
	const FVector ToTarget = (Target->GetActorLocation() - Pawn->GetActorLocation()).GetSafeNormal2D();
	if (ToTarget.IsNearlyZero())
	{
		// 같은 좌표(거의 없음). 발사 보류 — 다음 Tick 에 재계산.
		return;
	}

	const float DesiredYaw = ToTarget.Rotation().Yaw;
	const FRotator CurrentRot = Pawn->GetActorRotation();
	const float DeltaYaw = FMath::FindDeltaAngleDegrees(CurrentRot.Yaw, DesiredYaw);
	const float AbsDelta = FMath::Abs(DeltaYaw);

	// 회전 적용 — bRotatePawnToTarget=false 면 SetFocus 만 작동시키고 회전은 외부에 위임.
	if (bRotatePawnToTarget && AbsDelta > AlignmentToleranceDegrees)
	{
		const FRotator DesiredRot(CurrentRot.Pitch, DesiredYaw, CurrentRot.Roll);
		const FRotator NewRot = FMath::RInterpConstantTo(CurrentRot, DesiredRot, DeltaSeconds, RotationSpeedDegPerSec);
		Pawn->SetActorRotation(NewRot);
		AIController->SetControlRotation(FRotator(0.f, NewRot.Yaw, 0.f));
	}

	// 정렬 전엔 발사 보류 — 첫 발부터 일자 사선 보장. 사거리/쿨다운은 CombatComponent 내부에서 추가 검증.
	if (AbsDelta <= AlignmentToleranceDegrees)
	{
		// 우군이 사선에 끼면 발사 중단 → Failed 로 종료해 부모 Selector 가 Flank 분기로 이동.
		// BB 플래그(bFriendlyInLineOfFire) 갱신은 UpdateCombatBB Service 가 매 틱 담당.
		if (Combat->IsFriendlyInLineOfFire())
		{
			FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
			return;
		}

		Combat->RequestAttack();
	}
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
