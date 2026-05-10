#include "Enemy/AI/Controllers/PDEnemyAIControllerBase.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "Characters/Base/PDEnemyBase.h"
#include "DrawDebugHelpers.h"
#include "Enemy/AI/BehaviorTree/PDBTKeys.h"
#include "Enemy/Components/PDPerceptionComponent.h"
#include "Enemy/Components/PDCombatComponent.h"
#include "Enemy/Types/EnemyTypes.h"
#include "Engine/World.h"
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

void APDEnemyAIControllerBase::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	UE_LOG(LogPDAI, Log, TEXT("[%s] OnPossess: Pawn=%s, Perception=%s, BT=%s"),
		*GetNameSafe(this),
		*GetNameSafe(InPawn),
		GetPDPerception() ? TEXT("OK") : TEXT("MISSING"),
		BehaviorTreeAsset ? TEXT("OK") : TEXT("MISSING"));

	if (UPDPerceptionComponent* PDPerception = GetPDPerception())
	{
		PDPerception->OnTargetSpotted.AddDynamic(this, &APDEnemyAIControllerBase::HandleTargetSpotted);
		PDPerception->OnTargetLost   .AddDynamic(this, &APDEnemyAIControllerBase::HandleTargetLost);
		PDPerception->OnNoiseHeard   .AddDynamic(this, &APDEnemyAIControllerBase::HandleNoiseHeard);
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

	if (BehaviorTreeComponent)
	{
		BehaviorTreeComponent->StopTree(EBTStopMode::Safe);
	}

	Super::OnUnPossess();
}

void APDEnemyAIControllerBase::StartBehaviorTree()
{
	if (!BehaviorTreeAsset)
	{
		UE_LOG(LogPDAI, Warning, TEXT("[%s] BehaviorTreeAsset 미지정 — BT 가 실행되지 않음."), *GetNameSafe(this));
		return;
	}

	// RunBehaviorTree 는 BlackboardComponent 가 없으면 BT 자산의 BlackboardAsset 으로 생성/초기화.
	if (RunBehaviorTree(BehaviorTreeAsset))
	{
		UE_LOG(LogPDAI, Log, TEXT("[%s] RunBehaviorTree OK."), *GetNameSafe(this));
	}
	else
	{
		UE_LOG(LogPDAI, Warning, TEXT("[%s] RunBehaviorTree 실패 — BT 자산의 Blackboard 설정 확인."), *GetNameSafe(this));
	}
}

void APDEnemyAIControllerBase::HandleTargetSpotted(AActor* Target)
{
	UE_LOG(LogPDAI, Log, TEXT("[%s] HandleTargetSpotted: Target=%s"),
		*GetNameSafe(this), *GetNameSafe(Target));

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
	APawn* OwnerPawn = GetPawn();
	if (UPDCombatComponent* Combat = OwnerPawn ? OwnerPawn->FindComponentByClass<UPDCombatComponent>() : nullptr)
	{
		// 두 자극원 추적 중 한쪽만 Lost 됐을 때 다른 타겟이 날아가지 않도록 가드.
		if (Combat->GetCurrentTarget() == Target)
		{
			Combat->ClearCurrentTarget();
		}
	}

	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		// BT 는 LastSeenLocation 으로 Chase 진행 후, TargetActor=null 이면 Idle 복귀 분기.
		if (BB->GetValueAsObject(PDBTKeys::TargetActor) == Target)
		{
			BB->ClearValue(PDBTKeys::TargetActor);
			BB->SetValueAsVector(PDBTKeys::LastSeenLocation, LastKnownLocation);
		}
	}

	OnTargetLost(Target, LastKnownLocation);
}

void APDEnemyAIControllerBase::HandleNoiseHeard(AActor* NoiseInstigator, FVector Location)
{
	APawn* OwnerPawn = GetPawn();
	UPDCombatComponent* Combat = OwnerPawn ? OwnerPawn->FindComponentByClass<UPDCombatComponent>() : nullptr;

	const bool bHasVisualTarget = Combat && Combat->HasValidTarget();

	UE_LOG(LogPDAI, Log, TEXT("[%s] HandleNoiseHeard: Instigator=%s, Loc=%s, Suppressed(VisualTarget)=%s"),
		*GetNameSafe(this), *GetNameSafe(NoiseInstigator), *Location.ToString(),
		bHasVisualTarget ? TEXT("true") : TEXT("false"));

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
