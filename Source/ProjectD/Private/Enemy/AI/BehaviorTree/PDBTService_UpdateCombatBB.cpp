#include "Enemy/AI/BehaviorTree/PDBTService_UpdateCombatBB.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "DrawDebugHelpers.h"
#include "Enemy/AI/BehaviorTree/PDBTKeys.h"
#include "Enemy/AI/Controllers/PDEnemyAIControllerBase.h"
#include "Enemy/Components/PDCombatComponent.h"
#include "Enemy/Components/PDPerceptionComponent.h"
#include "Enemy/Interfaces/PDCombatInterface.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GenericTeamAgentInterface.h"
#include "HAL/IConsoleManager.h"
#include "Interfaces/PDDamageable.h"

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

	// ─── Stale Target 강제 해제 ─────────────────────────────────────────────
	// Squad 통보로 들어온 타겟은 perception 의 Lost 신호가 안 와서 영구 추적되는 버그를 차단.
	// 거리/생존 둘 중 하나라도 무효면 BB + Combat 양쪽 모두 clear → 아래 분기에서 Target=null 로 진행.
	if (AActor* CachedTarget = Cast<AActor>(BB->GetValueAsObject(PDBTKeys::TargetActor)))
	{
		const bool bDead = CachedTarget->Implements<UPDDamageable>()
			&& !IPDDamageable::Execute_IsAlive(CachedTarget);

		float MaxDist = MaxTrackDistance;
		if (MaxDist <= 0.f)
		{
			// AIController 의 perception 으로부터 LoseSightRadius 자동 채용 — perception 과 동일 정책.
			if (const APDEnemyAIControllerBase* PDController = Cast<APDEnemyAIControllerBase>(AIController))
			{
				if (const UPDPerceptionComponent* Perception = PDController->GetPDPerception())
				{
					MaxDist = Perception->GetLoseSightRadius();
				}
			}
		}
		MaxDist *= FMath::Max(MaxTrackDistanceMargin, 1.f);

		const float TargetDist = FVector::Dist(Pawn->GetActorLocation(), CachedTarget->GetActorLocation());
		const bool bOutOfRange = (MaxDist > 0.f) && (TargetDist > MaxDist);

		if (bDead || bOutOfRange)
		{
			// 해제 직전 마지막 위치 기록 — BT 가 LastSeenLocation 기반 investigate 분기로 자연 전이.
			BB->SetValueAsVector(PDBTKeys::LastSeenLocation, CachedTarget->GetActorLocation());
			BB->ClearValue(PDBTKeys::TargetActor);
			Combat->ClearCurrentTarget();
		}
	}

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

				// 시작점은 총구 우측 offset 반영(IsFriendlyInLineOfFire 와 동일 정책).
				const FVector TraceStart = Combat->GetFireTraceStart(0.f);
				const FVector TraceEnd   = Target->GetActorLocation();
				const bool bBlocked = World->LineTraceSingleByChannel(
					Hit, TraceStart, TraceEnd, ECC_Visibility, Params);

				if (!bBlocked)
				{
					bHasLOS = true;
				}
				else if (AActor* HitActor = Hit.GetActor())
				{
					// 차단자가 우군이면 '시야' 는 유효 — 발사 가능 여부는 bFriendlyInLineOfFire 에서 별도 판정.
					// 우군이 사선에 끼었다고 LastSeenLocation 으로 가버리는 분기 오인 차단.
					bool bBlockerIsFriendly = false;
					if (HitActor->Implements<UPDCombatInterface>() && Pawn->Implements<UPDCombatInterface>())
					{
						const uint8 OwnerTeam   = IPDCombatInterface::Execute_GetTeamID(Pawn);
						const uint8 BlockerTeam = IPDCombatInterface::Execute_GetTeamID(HitActor);
						bBlockerIsFriendly = (OwnerTeam != FGenericTeamId::NoTeam) && (OwnerTeam == BlockerTeam);
					}

					bHasLOS = bBlockerIsFriendly;
					Blocker = bBlockerIsFriendly ? nullptr : HitActor;
				}

#if ENABLE_DRAW_DEBUG
				if (const IConsoleVariable* CVar = GetPDAIDebugCVar())
				{
					if (CVar->GetInt() != 0)
					{
						// 초록 = LOS 확보, 노랑 = 우군 차단(LOS 는 유효 처리), 빨강 = 비-우군 차단.
						const FColor LineCol = bHasLOS
							? (bBlocked ? FColor::Yellow : FColor::Green)
							: FColor::Red;
						DrawDebugLine  (World, TraceStart, TraceEnd, LineCol, false, 0.2f, 0, 1.5f);
						DrawDebugSphere(World, TraceStart,    8.f, 8, FColor::Cyan,   false, 0.2f, 0, 1.0f);
						DrawDebugSphere(World, TraceEnd,      8.f, 8, FColor::Magenta,false, 0.2f, 0, 1.0f);
					}
				}
#endif
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
