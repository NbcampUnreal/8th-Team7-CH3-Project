#include "Enemy/AI/BehaviorTree/PDBTService_UpdateCombatBB.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Enemy/AI/BehaviorTree/PDBTKeys.h"
#include "Enemy/AI/Controllers/PDEnemyAIControllerBase.h"
#include "Enemy/Components/PDCombatComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "HAL/IConsoleManager.h"

#if ENABLE_DRAW_DEBUG
static IConsoleVariable* GetPDAIDebugCVar()
{
	static IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("pd.ai.debugdraw"));
	return CVar;
}
#endif

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
	AActor* Blocker = nullptr;
	float   Dist    = -1.f;

	if (Target)
	{
		const float Range = Combat->GetAttackRange();
		Dist = FVector::Dist(Pawn->GetActorLocation(), Target->GetActorLocation());

		if (Range > 0.f)
		{
			bInRange = Dist <= Range;
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
				bHasLOS  = !bBlocked;
				Blocker  = bBlocked ? Hit.GetActor() : nullptr;
			}
		}
	}

	BB->SetValueAsBool(PDBTKeys::IsTargetInRange, bInRange);
	BB->SetValueAsBool(PDBTKeys::HasLOSToTarget,  bHasLOS);

	// 우군 사선 평가는 매 틱 갱신 — 우군이 사선에서 빠지면 자동으로 false 로 복귀해
	// BT 데코레이터가 공격 분기로 재진입 가능. 타겟이 없으면 false 로 강제.
	const bool bFriendlyInLOF = Target ? Combat->IsFriendlyInLineOfFire() : false;
	BB->SetValueAsBool(PDBTKeys::bFriendlyInLineOfFire, bFriendlyInLOF);

#if ENABLE_DRAW_DEBUG
	// pd.ai.debugdraw 1 일 때만 LOS 상태 출력 — 0.2s 마다 1줄.
	if (Target)
	{
		const IConsoleVariable* CVar = GetPDAIDebugCVar();
		if (CVar && CVar->GetInt() != 0)
		{
			UE_LOG(LogPDAI, Log,
				TEXT("[%s] LOS: Target=%s, Dist=%.0f, Range=%.0f, InRange=%s, HasLOS=%s, Blocker=%s"),
				*GetNameSafe(Pawn),
				*GetNameSafe(Target),
				Dist,
				Combat->GetAttackRange(),
				bInRange ? TEXT("Y") : TEXT("N"),
				bHasLOS  ? TEXT("Y") : TEXT("N"),
				*GetNameSafe(Blocker));
		}
	}
#endif

	// 시각 타겟이 잡히면 NoiseHint 는 정보가치 낮음 — 자동 만료.
	if (Combat->HasValidTarget() && Combat->HasNoiseHint())
	{
		Combat->ClearNoiseHint();
		BB->SetValueAsBool(PDBTKeys::HasNoiseHint, false);
	}
}
