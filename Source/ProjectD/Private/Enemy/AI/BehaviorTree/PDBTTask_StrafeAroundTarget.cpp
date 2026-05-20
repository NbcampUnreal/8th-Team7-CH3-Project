#include "Enemy/AI/BehaviorTree/PDBTTask_StrafeAroundTarget.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Enemy/AI/BehaviorTree/PDBTKeys.h"
#include "GameFramework/Pawn.h"

UPDBTTask_StrafeAroundTarget::UPDBTTask_StrafeAroundTarget()
{
	NodeName = TEXT("PD Strafe Around Target");
	BlackboardKey.SelectedKeyName = PDBTKeys::TargetActor;

	bNotifyTick = true;
	bNotifyTaskFinished = true;
}

uint16 UPDBTTask_StrafeAroundTarget::GetInstanceMemorySize() const
{
	return sizeof(FPDStrafeTaskMemory);
}

EBTNodeResult::Type UPDBTTask_StrafeAroundTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	AAIController* AI = OwnerComp.GetAIOwner();
	APawn* Pawn = AI ? AI->GetPawn() : nullptr;
	AActor* Target = BB ? Cast<AActor>(BB->GetValueAsObject(BlackboardKey.SelectedKeyName)) : nullptr;

	if (!Pawn || !Target) return EBTNodeResult::Failed;

	FPDStrafeTaskMemory* Mem = reinterpret_cast<FPDStrafeTaskMemory*>(NodeMemory);
	Mem->ElapsedTime = 0.f;

	switch (Direction)
	{
		case EPDStrafeDirection::Left:  Mem->SideSign = +1; break;
		case EPDStrafeDirection::Right: Mem->SideSign = -1; break;
		case EPDStrafeDirection::Random:
		default:                        Mem->SideSign = FMath::RandBool() ? +1 : -1; break;
	}

	// 조준 잠금 — Task 진행 동안 폰이 적을 계속 바라봄.
	AI->SetFocus(Target, EAIFocusPriority::Gameplay);

	return EBTNodeResult::InProgress;
}

void UPDBTTask_StrafeAroundTarget::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	AAIController* AI = OwnerComp.GetAIOwner();
	APawn* Pawn = AI ? AI->GetPawn() : nullptr;
	AActor* Target = BB ? Cast<AActor>(BB->GetValueAsObject(BlackboardKey.SelectedKeyName)) : nullptr;

	if (!Pawn || !Target)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	FPDStrafeTaskMemory* Mem = reinterpret_cast<FPDStrafeTaskMemory*>(NodeMemory);
	Mem->ElapsedTime += DeltaSeconds;

	if (Mem->ElapsedTime >= Duration)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	const FVector PawnLoc = Pawn->GetActorLocation();
	const FVector TargetLoc = Target->GetActorLocation();
	const FVector ToTarget2D = FVector(TargetLoc.X - PawnLoc.X, TargetLoc.Y - PawnLoc.Y, 0.f);
	const float Distance = ToTarget2D.Size();

	if (Distance < KINDA_SMALL_NUMBER) return;

	const FVector ToTargetDir = ToTarget2D / Distance;

	// Lateral = ToTarget × Up — 적 방향에 수직인 좌/우 단위벡터.
	const FVector LateralDir = FVector::CrossProduct(ToTargetDir, FVector::UpVector) * Mem->SideSign;
	Pawn->AddMovementInput(LateralDir, LateralInputScale);

	// 거리 유지 — DesiredRange 보다 가까우면 후퇴, 멀면 전진. Tolerance 안이면 보정 없음.
	if (Distance < DesiredRange - DistanceTolerance)
	{
		Pawn->AddMovementInput(-ToTargetDir, DistanceCorrectScale);
	}
	else if (Distance > DesiredRange + DistanceTolerance)
	{
		Pawn->AddMovementInput(ToTargetDir, DistanceCorrectScale);
	}
}

EBTNodeResult::Type UPDBTTask_StrafeAroundTarget::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	// ClearFocus 는 OnTaskFinished 에서 일괄 처리.
	Super::AbortTask(OwnerComp, NodeMemory);
	return EBTNodeResult::Aborted;
}

void UPDBTTask_StrafeAroundTarget::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);

	if (AAIController* AI = OwnerComp.GetAIOwner())
	{
		AI->ClearFocus(EAIFocusPriority::Gameplay);
	}
}
