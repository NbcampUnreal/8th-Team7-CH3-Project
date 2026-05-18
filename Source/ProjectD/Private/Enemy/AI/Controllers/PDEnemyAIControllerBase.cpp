#include "Enemy/AI/Controllers/PDEnemyAIControllerBase.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "Characters/Base/PDEnemyBase.h"
#include "Components/SplineComponent.h"
#include "DrawDebugHelpers.h"
#include "Enemy/AI/BehaviorTree/PDBTKeys.h"
#include "Enemy/Characters/PDBipedEnemy.h"
#include "Enemy/Components/PDPerceptionComponent.h"
#include "Enemy/Components/PDCombatComponent.h"
#include "GameFramework/Controller.h"
#include "Enemy/Types/EnemyTypes.h"
#include "Engine/World.h"
#include "GenericTeamAgentInterface.h"
#include "HAL/IConsoleManager.h"
#include "Navigation/PathFollowingComponent.h"

DEFINE_LOG_CATEGORY(LogPDAI);

#if ENABLE_DRAW_DEBUG
static TAutoConsoleVariable<int32> CVarPDAIDebugDraw(
	TEXT("pd.ai.debugdraw"),
	0,
	TEXT("AI 시야/이동 디버그 시각화. 0=off, 1=on."),
	ECVF_Cheat
);
#endif

APDEnemyAIControllerBase::APDEnemyAIControllerBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 부모 AAIController 의 PerceptionComponent 슬롯에 우리 파생 컴포넌트를 주입.
	PerceptionComponent = CreateDefaultSubobject<UPDPerceptionComponent>(TEXT("PDPerception"));

	// BT 실행 컴포넌트. Blackboard 는 RunBehaviorTree 시 UseBlackboard 로 부모 슬롯에 자동 주입.
	BehaviorTreeComponent = CreateDefaultSubobject<UBehaviorTreeComponent>(TEXT("BehaviorTree"));

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

UPDPerceptionComponent* APDEnemyAIControllerBase::GetPDPerception() const
{
	return Cast<UPDPerceptionComponent>(PerceptionComponent);
}

ETeamAttitude::Type APDEnemyAIControllerBase::GetTeamAttitudeTowards(const AActor& Other) const
{
	// 1) Pawn 자체 → 2) Pawn 의 Controller 순으로 팀 해석.
	//    AAIController 디폴트는 Controller 우선이라 Pawn 점유 직후 타이밍에 NoTeam(255) 으로 평가될 수 있어
	//    여기서는 Pawn 우선으로 뒤집어 안정성 확보.
	const IGenericTeamAgentInterface* OtherTeam = Cast<IGenericTeamAgentInterface>(&Other);
	if (!OtherTeam)
	{
		if (const APawn* OtherPawn = Cast<APawn>(&Other))
		{
			OtherTeam = Cast<IGenericTeamAgentInterface>(OtherPawn->GetController());
		}
	}
	if (!OtherTeam) return ETeamAttitude::Neutral;

	const FGenericTeamId MyId    = GetGenericTeamId();
	const FGenericTeamId OtherId = OtherTeam->GetGenericTeamId();
	if (MyId == FGenericTeamId::NoTeam || OtherId == FGenericTeamId::NoTeam) return ETeamAttitude::Neutral;

	return (MyId == OtherId) ? ETeamAttitude::Friendly : ETeamAttitude::Hostile;
}

void APDEnemyAIControllerBase::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// Perception 친화도 평가는 Controller 의 GenericTeamId 를 기준으로 함.
	// Pawn 의 TeamID 를 그대로 전파하지 않으면 Controller 가 NoTeam 으로 남아
	// 같은 팀(Soldier↔Soldier)이 서로 적/중립으로 잘못 판정될 수 있다.
	if (const IGenericTeamAgentInterface* PawnTeam = Cast<IGenericTeamAgentInterface>(InPawn))
	{
		SetGenericTeamId(PawnTeam->GetGenericTeamId());
	}

	if (UPDPerceptionComponent* PDPerception = GetPDPerception())
	{
		PDPerception->OnTargetSpotted.AddDynamic(this, &APDEnemyAIControllerBase::HandleTargetSpotted);
		PDPerception->OnTargetLost   .AddDynamic(this, &APDEnemyAIControllerBase::HandleTargetLost);
		PDPerception->OnNoiseHeard   .AddDynamic(this, &APDEnemyAIControllerBase::HandleNoiseHeard);
	}

	// Squad 통보(NotifyAlliesInRadius) 등 외부 경로로 들어온 타겟도 BB 에 반영하도록 구독.
	if (UPDCombatComponent* Combat = InPawn ? InPawn->FindComponentByClass<UPDCombatComponent>() : nullptr)
	{
		Combat->OnTargetChanged.AddDynamic(this, &APDEnemyAIControllerBase::HandleCombatTargetChanged);
	}

	StartBehaviorTree();

	// HomeLocation 초기화 — Patrol Task 가 기준점으로 사용.
	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		BB->SetValueAsVector(PDBTKeys::HomeLocation, InPawn ? InPawn->GetActorLocation() : FVector::ZeroVector);
		BB->SetValueAsEnum  (PDBTKeys::EnemyState,  static_cast<uint8>(EPDEnemyState::Idle));

		// AttackRange / PatrolRadius 는 디자이너가 BB 디폴트에서 지정 가능 — 여기선 강제 덮어쓰지 않음.
	}
}

void APDEnemyAIControllerBase::OnUnPossess()
{
	if (UPDPerceptionComponent* PDPerception = GetPDPerception())
	{
		PDPerception->OnTargetSpotted.RemoveAll(this);
		PDPerception->OnTargetLost   .RemoveAll(this);
		PDPerception->OnNoiseHeard   .RemoveAll(this);
	}

	if (APawn* OwnerPawn = GetPawn())
	{
		if (UPDCombatComponent* Combat = OwnerPawn->FindComponentByClass<UPDCombatComponent>())
		{
			Combat->OnTargetChanged.RemoveAll(this);
		}
	}

	if (BehaviorTreeComponent)
	{
		BehaviorTreeComponent->StopTree(EBTStopMode::Safe);
	}

	Super::OnUnPossess();
}

void APDEnemyAIControllerBase::NotifyPawnDied()
{
	// 사망 즉시 의사결정 중단 — Perception 구독/팀ID는 보존(액터 소멸 시 OnUnPossess 가 일괄 해제).
	if (BehaviorTreeComponent && BehaviorTreeComponent->IsRunning())
	{
		BehaviorTreeComponent->StopTree(EBTStopMode::Safe);
	}
}

void APDEnemyAIControllerBase::StartBehaviorTree()
{
	if (!BehaviorTreeAsset) return;

	// RunBehaviorTree 는 BlackboardComponent 가 없으면 BT 자산의 BlackboardAsset 으로 생성/초기화.
	RunBehaviorTree(BehaviorTreeAsset);
}

void APDEnemyAIControllerBase::HandleTargetSpotted(AActor* Target)
{
	// CombatComponent 결합은 AIController 가 담당 → BT 는 BB 만 보고 단순히 분기.
	APawn* OwnerPawn = GetPawn();
	if (UPDCombatComponent* Combat = OwnerPawn ? OwnerPawn->FindComponentByClass<UPDCombatComponent>() : nullptr)
	{
		Combat->SetCurrentTarget(Target);
	}

	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		BB->SetValueAsObject(PDBTKeys::TargetActor, Target);
		if (Target)
		{
			BB->SetValueAsVector(PDBTKeys::LastSeenLocation, Target->GetActorLocation());
		}
	}

	OnTargetSpotted(Target);
}

void APDEnemyAIControllerBase::HandleTargetLost(AActor* Target, FVector LastKnownLocation)
{
	// 순서 주의: BB 먼저, Combat 나중.
	// Combat->ClearCurrentTarget 이 OnTargetChanged.Broadcast(nullptr) 를 통해 HandleCombatTargetChanged 를 즉시 호출하고
	// 그 안에서 BB.TargetActor 가 null 처리됨. 그 뒤에 본 함수의 BB 검사를 하면 `== Target` 비교가 false 가 되어
	// LastSeenLocation 갱신이 영구 SKIP 되는 순서 버그 회피.
	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		if (BB->GetValueAsObject(PDBTKeys::TargetActor) == Target)
		{
			BB->ClearValue(PDBTKeys::TargetActor);
			BB->SetValueAsVector(PDBTKeys::LastSeenLocation, LastKnownLocation);
		}
	}

	APawn* OwnerPawn = GetPawn();
	if (UPDCombatComponent* Combat = OwnerPawn ? OwnerPawn->FindComponentByClass<UPDCombatComponent>() : nullptr)
	{
		// 두 자극원 추적 중 한쪽만 Lost 됐을 때 다른 타겟이 날아가지 않도록 가드.
		if (Combat->GetCurrentTarget() == Target)
		{
			Combat->ClearCurrentTarget();
		}
	}

	OnTargetLost(Target, LastKnownLocation);
}

void APDEnemyAIControllerBase::HandleCombatTargetChanged(AActor* NewTarget)
{
	// Combat 측에서 타겟이 set/clear 될 때마다 BB 와 동기화.
	// HandleTargetSpotted 가 Perception 발견 시 이미 BB 갱신을 하므로 이 경로는 멱등 처리.
	// 의의: Squad 통보(NotifyAlliesInRadius) 등 Perception 을 거치지 않는 외부 경로의 타겟도 BT 에 반영.
	UBlackboardComponent* BB = GetBlackboardComponent();
	if (!BB) return;

	if (NewTarget)
	{
		BB->SetValueAsObject(PDBTKeys::TargetActor, NewTarget);
		BB->SetValueAsVector(PDBTKeys::LastSeenLocation, NewTarget->GetActorLocation());
	}
	else
	{
		BB->ClearValue(PDBTKeys::TargetActor);
	}
}

void APDEnemyAIControllerBase::HandleNoiseHeard(AActor* NoiseInstigator, FVector Location)
{
	APawn* OwnerPawn = GetPawn();
	UPDCombatComponent* Combat = OwnerPawn ? OwnerPawn->FindComponentByClass<UPDCombatComponent>() : nullptr;

	const bool bHasVisualTarget = Combat && Combat->HasValidTarget();

	const uint8 SelfTeam        = GetGenericTeamId().GetId();
	const uint8 InstigatorTeam  = FGenericTeamId::GetTeamIdentifier(NoiseInstigator).GetId();
	const bool  bSameTeam       = (SelfTeam != FGenericTeamId::NoTeam)
	                              && (InstigatorTeam != FGenericTeamId::NoTeam)
	                              && (SelfTeam == InstigatorTeam);

	// Affiliation 필터 통과 후라도 같은 팀이면 노이즈 힌트 갱신 차단.
	if (bSameTeam) { OnNoiseHeard(NoiseInstigator, Location); return; }

	// 시각 타겟이 있으면 청각은 정보가치 낮음 — skip.
	if (bHasVisualTarget) { OnNoiseHeard(NoiseInstigator, Location); return; }

	if (Combat)
	{
		Combat->SetLastNoiseLocation(NoiseInstigator, Location);
	}

	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		BB->SetValueAsBool(PDBTKeys::HasNoiseHint, true);
		BB->SetValueAsVector(PDBTKeys::LastNoiseLocation, Location);
	}

	OnNoiseHeard(NoiseInstigator, Location);
}

void APDEnemyAIControllerBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

#if ENABLE_DRAW_DEBUG
	if (CVarPDAIDebugDraw.GetValueOnGameThread() != 0)
	{
		DrawAIDebug();
	}
#endif
}

void APDEnemyAIControllerBase::DrawAIDebug() const
{
#if ENABLE_DRAW_DEBUG
	UWorld* World = GetWorld();
	APawn* OwnerPawn = GetPawn();
	if (!World || !OwnerPawn) return;

	const FVector Origin = OwnerPawn->GetActorLocation();
	const FVector Forward = OwnerPawn->GetActorForwardVector();

	if (const UPDPerceptionComponent* Perception = GetPDPerception())
	{
		const float SightRadius = Perception->GetSightRadius();
		const float LoseSightRadius = FMath::Max(Perception->GetLoseSightRadius(), SightRadius);
		const float HalfAngleRad = FMath::DegreesToRadians(Perception->GetPeripheralVisionAngleDegrees());

		DrawDebugCone(World, Origin, Forward, SightRadius, HalfAngleRad, HalfAngleRad,
			24, FColor::Green, false, -1.f, SDPG_World, 1.f);
		DrawDebugCone(World, Origin, Forward, LoseSightRadius, HalfAngleRad, HalfAngleRad,
			24, FColor(255, 200, 0), false, -1.f, SDPG_World, 0.5f);

		const float HearingRange = Perception->GetHearingRange();
		DrawDebugCircle(World, Origin, HearingRange, 48, FColor::Cyan, false, -1.f, SDPG_World, 1.f,
			FVector(1.f, 0.f, 0.f), FVector(0.f, 1.f, 0.f), false);
	}

	UPDCombatComponent* Combat = OwnerPawn->FindComponentByClass<UPDCombatComponent>();
	if (Combat)
	{
		if (AActor* Target = Combat->GetCurrentTarget())
		{
			const FVector TargetLoc = Target->GetActorLocation();
			DrawDebugLine  (World, Origin, TargetLoc, FColor::Red, false, -1.f, SDPG_World, 2.f);
			DrawDebugSphere(World, TargetLoc, 40.f, 12, FColor::Red, false, -1.f, SDPG_World, 1.5f);
		}

		if (Combat->HasNoiseHint())
		{
			const FVector NoiseLoc = Combat->GetLastNoiseLocation();
			DrawDebugSphere(World, NoiseLoc, 60.f, 16, FColor::Yellow, false, -1.f, SDPG_World, 2.f);
			DrawDebugLine  (World, Origin, NoiseLoc, FColor::Yellow, false, -1.f, SDPG_World, 1.f);
		}
	}

	// Patrol waypoints 시각화 — bUsePatrolRoute=true 인 BipedEnemy 의 캐시된 월드 좌표.
	// 에디터의 spline 라인은 적과 함께 움직이지만, 캐시된 점들은 런타임 고정 좌표라 patrol 의 진짜 위치 확인용.
	if (const APDBipedEnemy* Biped = Cast<APDBipedEnemy>(OwnerPawn))
	{
		if (Biped->HasPatrolRoute())
		{
			const FVector Lift(0.f, 0.f, 30.f);
			TArray<FVector> Pts;
			Pts.Reserve(8);
			// PDBipedEnemy 의 캐시는 private — public 헬퍼 없이도 spline 컴포넌트에서 다시 읽어와 시각화.
			if (const USplineComponent* Spline = Biped->GetPatrolRouteSpline())
			{
				for (int32 i = 0; i < Spline->GetNumberOfSplinePoints(); ++i)
				{
					Pts.Add(Spline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World));
				}
			}
			for (int32 i = 0; i < Pts.Num(); ++i)
			{
				DrawDebugSphere(World, Pts[i] + Lift, 30.f, 12, FColor(0, 200, 255), false, -1.f, SDPG_World, 1.5f);
				const FString Label = FString::Printf(TEXT("WP%d"), i);
				DrawDebugString(World, Pts[i] + Lift + FVector(0, 0, 30), Label, nullptr, FColor::White, 0.f, false, 1.f);
			}
			for (int32 i = 1; i < Pts.Num(); ++i)
			{
				DrawDebugLine(World, Pts[i - 1] + Lift, Pts[i] + Lift, FColor(0, 200, 255), false, -1.f, SDPG_World, 2.f);
			}
		}
	}

	if (const UPathFollowingComponent* PF = GetPathFollowingComponent())
	{
		const FNavPathSharedPtr Path = PF->GetPath();
		if (Path.IsValid() && Path->IsValid())
		{
			const TArray<FNavPathPoint>& Points = Path->GetPathPoints();
			constexpr float ZLift = 10.f;
			for (int32 i = 1; i < Points.Num(); ++i)
			{
				DrawDebugLine(World,
					Points[i - 1].Location + FVector(0, 0, ZLift),
					Points[i].Location     + FVector(0, 0, ZLift),
					FColor::Magenta, false, -1.f, SDPG_World, 3.f);
			}
		}
	}
#endif
}
