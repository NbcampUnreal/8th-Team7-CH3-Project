#include "Enemy/AI/BehaviorTree/PDBTTask_MeleeAttack.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Enemy/Components/PDCombatComponent.h"
#include "GameFramework/Pawn.h"

namespace
{
	struct FPDMeleeAttackMemory
	{
		float ElapsedTime = 0.f;
	};
}

UPDBTTask_MeleeAttack::UPDBTTask_MeleeAttack()
{
	NodeName = TEXT("PD Melee Attack");
	bNotifyTick = true;
}

uint16 UPDBTTask_MeleeAttack::GetInstanceMemorySize() const
{
	return sizeof(FPDMeleeAttackMemory);
}

EBTNodeResult::Type UPDBTTask_MeleeAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* Pawn = AIController ? AIController->GetPawn() : nullptr;
	if (!Pawn) return EBTNodeResult::Failed;

	UPDCombatComponent* Combat = Pawn->FindComponentByClass<UPDCombatComponent>();
	if (!Combat || !Combat->HasValidTarget()) return EBTNodeResult::Failed;

	AActor* Target = Combat->GetCurrentTarget();
	if (!Target) return EBTNodeResult::Failed;

	// 즉시 정렬 — 휘두름 동작 직전 한 번만. 보간 없이 commit.
	if (bSnapRotateToTarget)
	{
		const FVector ToTarget = (Target->GetActorLocation() - Pawn->GetActorLocation()).GetSafeNormal2D();
		if (!ToTarget.IsNearlyZero())
		{
			const FRotator DesiredRot(0.f, ToTarget.Rotation().Yaw, 0.f);
			Pawn->SetActorRotation(DesiredRot);
			AIController->SetControlRotation(DesiredRot);
		}
	}

	// RequestAttack — CombatComponent 가 거리/쿨다운 검증 후 OnAttackRequested 브로드캐스트.
	// 실패 시 즉시 Failed → 부모 Selector 가 Chase 분기 등으로 복귀.
	if (!Combat->RequestAttack())
	{
		return EBTNodeResult::Failed;
	}

	FPDMeleeAttackMemory* Memory = new (NodeMemory) FPDMeleeAttackMemory();
	Memory->ElapsedTime = 0.f;

	return EBTNodeResult::InProgress;
}

void UPDBTTask_MeleeAttack::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	FPDMeleeAttackMemory* Memory = reinterpret_cast<FPDMeleeAttackMemory*>(NodeMemory);
	if (!Memory)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	if (bAbortIfTargetLost)
	{
		AAIController* AIController = OwnerComp.GetAIOwner();
		APawn* Pawn = AIController ? AIController->GetPawn() : nullptr;
		UPDCombatComponent* Combat = Pawn ? Pawn->FindComponentByClass<UPDCombatComponent>() : nullptr;
		if (!Combat || !Combat->HasValidTarget())
		{
			FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
			return;
		}
	}

	Memory->ElapsedTime += DeltaSeconds;
	if (Memory->ElapsedTime >= AttackDuration)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}

EBTNodeResult::Type UPDBTTask_MeleeAttack::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* /*NodeMemory*/)
{
	// 진행 중인 GA 는 별도 취소하지 않음 — 휘두름은 committed action.
	// 명시적 취소가 필요하면 BP 측에서 CancelAbilitiesByTag 호출로 처리.
	return EBTNodeResult::Aborted;
}
