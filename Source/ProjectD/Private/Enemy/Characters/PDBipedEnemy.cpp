#include "Enemy/Characters/PDBipedEnemy.h"

#include "AttributeSet/PDAttributeSet.h"
#include "Components/SplineComponent.h"
#include "Enemy/AI/Controllers/PDEnemyAIControllerBase.h"
#include "Enemy/Components/PDCombatComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

APDBipedEnemy::APDBipedEnemy()
{
	CombatComponent = CreateDefaultSubobject<UPDCombatComponent>(TEXT("CombatComponent"));

	// 동료끼리 서로 막지 않도록 RVO 회피 활성화 — Detour Crowd 컨트롤러 없이도 작동.
	// AvoidanceConsiderationRadius 는 캡슐 반경의 ~5배가 일반적. 디자이너가 BP 에서 튜닝 가능.
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->bUseRVOAvoidance = true;
		MoveComp->AvoidanceWeight = 0.5f;
		MoveComp->AvoidanceConsiderationRadius = 250.f;
	}

	// Patrol 경로 spline — 인스턴스별 편집 가능. 루트에 부착해 디자인 시 적과 같이 따라다님.
	// 게임플레이 중엔 BeginPlay 에서 좌표 캐싱 후 spline 의존을 끊어 적이 patrol 점을 끌고 다니지 않음.
	PatrolRouteSpline = CreateDefaultSubobject<USplineComponent>(TEXT("PatrolRouteSpline"));
	if (PatrolRouteSpline)
	{
		PatrolRouteSpline->SetupAttachment(GetRootComponent());
		PatrolRouteSpline->SetMobility(EComponentMobility::Movable);

		// 초기 4개 점 — 디자이너가 viewport 에서 드래그해 조정.
		PatrolRouteSpline->ClearSplinePoints(false);
		PatrolRouteSpline->AddSplinePoint(FVector(   0.f,    0.f, 0.f), ESplineCoordinateSpace::Local, false);
		PatrolRouteSpline->AddSplinePoint(FVector( 400.f,    0.f, 0.f), ESplineCoordinateSpace::Local, false);
		PatrolRouteSpline->AddSplinePoint(FVector( 400.f,  400.f, 0.f), ESplineCoordinateSpace::Local, false);
		PatrolRouteSpline->AddSplinePoint(FVector(   0.f,  400.f, 0.f), ESplineCoordinateSpace::Local, false);
		PatrolRouteSpline->UpdateSpline();

		// 충돌/오버랩 비활성 — 시각/편집 전용.
		PatrolRouteSpline->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		PatrolRouteSpline->SetGenerateOverlapEvents(false);

		// PIE 중엔 spline 라인 숨김 — 디자인 시간에만 보이도록.
		PatrolRouteSpline->bDrawDebug = true;
		PatrolRouteSpline->bShouldVisualizeScale = false;
	}
}

void APDBipedEnemy::BeginPlay()
{
	Super::BeginPlay();

	// 스폰 시점의 spline 월드 좌표를 캐싱 — 이후 적이 어디로 이동하든 patrol 점은 고정.
	CachedPatrolWaypoints.Reset();
	if (PatrolRouteSpline)
	{
		const int32 NumPoints = PatrolRouteSpline->GetNumberOfSplinePoints();
		CachedPatrolWaypoints.Reserve(NumPoints);
		for (int32 i = 0; i < NumPoints; ++i)
		{
			CachedPatrolWaypoints.Add(PatrolRouteSpline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World));
		}
	}

	CurrentPatrolIndex = 0;
	PatrolStepDir = 1;

	// 진단 로그 — bUsePatrolRoute 토글 / 점 개수 / 좌표를 PIE 시작 시 한 번 출력해 wiring 검증.
	UE_LOG(LogPDAI, Warning,
		TEXT("[%s] Patrol init — bUsePatrolRoute=%s, bLoop=%s, CachedPoints=%d"),
		*GetName(),
		bUsePatrolRoute ? TEXT("TRUE") : TEXT("FALSE"),
		bLoopPatrolRoute ? TEXT("loop") : TEXT("ping-pong"),
		CachedPatrolWaypoints.Num());
	for (int32 i = 0; i < CachedPatrolWaypoints.Num(); ++i)
	{
		const FVector& P = CachedPatrolWaypoints[i];
		UE_LOG(LogPDAI, Warning, TEXT("    [%d] World=(%.0f, %.0f, %.0f)"), i, P.X, P.Y, P.Z);
	}
}

bool APDBipedEnemy::GetNextPatrolWaypoint(FVector& OutLocation)
{
	const int32 N = CachedPatrolWaypoints.Num();
	if (!bUsePatrolRoute)
	{
		UE_LOG(LogPDAI, Verbose, TEXT("[%s] GetNextPatrolWaypoint FAIL — bUsePatrolRoute=false (BP default. BB 키 아님)"),
			*GetName());
		return false;
	}
	if (N < 2)
	{
		UE_LOG(LogPDAI, Warning, TEXT("[%s] GetNextPatrolWaypoint FAIL — spline 점 부족 (N=%d). PatrolRouteSpline 에 점 ≥2 필요"),
			*GetName(), N);
		return false;
	}

	// 현재 인덱스 좌표 반환 후, 인덱스를 다음 점으로 진행 — 다음 호출은 다음 점.
	CurrentPatrolIndex = FMath::Clamp(CurrentPatrolIndex, 0, N - 1);
	OutLocation = CachedPatrolWaypoints[CurrentPatrolIndex];

	UE_LOG(LogPDAI, Verbose,
		TEXT("[%s] GetNextPatrolWaypoint → idx=%d/%d World=(%.0f, %.0f, %.0f)"),
		*GetName(), CurrentPatrolIndex, N - 1, OutLocation.X, OutLocation.Y, OutLocation.Z);

	if (bLoopPatrolRoute)
	{
		CurrentPatrolIndex = (CurrentPatrolIndex + 1) % N;
	}
	else
	{
		// Ping-pong: 끝에 닿으면 방향 반전 후 다음 점.
		const int32 NextIndex = CurrentPatrolIndex + PatrolStepDir;
		if (NextIndex < 0 || NextIndex >= N)
		{
			PatrolStepDir *= -1;
			CurrentPatrolIndex = FMath::Clamp(CurrentPatrolIndex + PatrolStepDir, 0, N - 1);
		}
		else
		{
			CurrentPatrolIndex = NextIndex;
		}
	}
	return true;
}

EPDStaminaStatus APDBipedEnemy::GetStaminaStatus_Implementation() const
{
	// AttributeSet 미초기화/Max=0 이면 Stamina 미사용으로 간주.
	if (!AttributeSet) return EPDStaminaStatus::None;

	const float MaxS = AttributeSet->GetMaxStamina();
	if (MaxS <= 0.f) return EPDStaminaStatus::None;

	const float Percent = AttributeSet->GetStamina() / MaxS;

	if (Percent >= StaminaFullThreshold)    return EPDStaminaStatus::Full;
	if (Percent >= StaminaOptimalThreshold) return EPDStaminaStatus::Optimal;
	if (Percent >= StaminaLowThreshold)     return EPDStaminaStatus::Low;
	return EPDStaminaStatus::Exhausted;
}
