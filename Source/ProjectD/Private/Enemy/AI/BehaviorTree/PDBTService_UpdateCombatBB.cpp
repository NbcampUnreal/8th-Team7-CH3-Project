#include "Enemy/AI/BehaviorTree/PDBTService_UpdateCombatBB.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Enemy/AI/BehaviorTree/PDBTKeys.h"
#include "Enemy/Components/PDCombatComponent.h"
#include "Engine/World.h"

UPDBTService_UpdateCombatBB::UPDBTService_UpdateCombatBB()
{
	NodeName = TEXT("PD Update Combat BB");
	Interval = 0.2f;
	RandomDeviation = 0.05f;

	bNotifyTick = true;
	bTickIntervals = true;
}

void UPDBTService_UpdateCombatBB::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* Pawn = AIController ? AIController->GetPawn() : nullptr;
	UPDCombatComponent* Combat = Pawn ? Pawn->FindComponentByClass<UPDCombatComponent>() : nullptr;
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!Combat || !BB) return;

	BB->SetValueAsBool (PDBTKeys::CanAttack,    Combat->CanAttack());
	BB->SetValueAsFloat(PDBTKeys::AttackRange,  Combat->GetAttackRange());

	// 거리/시야는 매 프레임 변하지만 BB 키 자체는 안 바뀌므로 Decorator 옵저버가 트리거되지 않음.
	// 결과를 Bool 키에 캐싱해서 표준 Blackboard Decorator 가 변화를 감지하도록 함.
	AActor* Target = Cast<AActor>(BB->GetValueAsObject(PDBTKeys::TargetActor));
	bool bInRange = false;
	bool bHasLOS  = false;

	if (Target)
	{
		const float Range = Combat->GetAttackRange();
		if (Range > 0.f)
		{
			const float DistSq = FVector::DistSquared(Pawn->GetActorLocation(), Target->GetActorLocation());
			bInRange = DistSq <= (Range * Range);
		}

		// 사거리 밖이면 LineTrace 비용 절약.
		if (bInRange)
		{
			if (UWorld* World = GetWorld())
			{
				FHitResult Hit;
				FCollisionQueryParams Params(SCENE_QUERY_STAT(PD_BT_LOS), true, Pawn);
				Params.AddIgnoredActor(Target);
				const bool bBlocked = World->LineTraceSingleByChannel(
					Hit, Pawn->GetActorLocation(), Target->GetActorLocation(), ECC_Visibility, Params);
				bHasLOS = !bBlocked;
			}
		}
	}

	BB->SetValueAsBool(PDBTKeys::IsTargetInRange, bInRange);
	BB->SetValueAsBool(PDBTKeys::HasLOSToTarget,  bHasLOS);

	// 시각 타겟이 잡히면 NoiseHint 는 정보가치 낮음 — 자동 만료.
	if (Combat->HasValidTarget() && Combat->HasNoiseHint())
	{
		Combat->ClearNoiseHint();
		BB->SetValueAsBool(PDBTKeys::HasNoiseHint, false);
	}
}
