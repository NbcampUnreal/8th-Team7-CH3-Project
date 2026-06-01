#include "Enemy/Components/PDCombatComponent.h"

#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "HAL/IConsoleManager.h"
#include "Enemy/Interfaces/PDCombatInterface.h"
#include "GenericTeamAgentInterface.h"
#include "Interfaces/PDDamageable.h"

#if ENABLE_DRAW_DEBUG
// pd.ai.debugdraw 와 동일 CVar 를 참조 — 정의는 PDEnemyAIControllerBase.cpp 에 있음.
static IConsoleVariable* GetPDAIDebugDrawCVar()
{
	static IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("pd.ai.debugdraw"));
	return CVar;
}
#endif

UPDCombatComponent::UPDCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPDCombatComponent::SetCurrentTarget(AActor* NewTarget)
{
	if (CurrentTarget.Get() == NewTarget) return;

	CurrentTarget = NewTarget;
	OnTargetChanged.Broadcast(NewTarget);
}

void UPDCombatComponent::ClearCurrentTarget()
{
	if (!CurrentTarget.IsValid() && CurrentTarget.IsExplicitlyNull())
	{
		return;
	}
	CurrentTarget.Reset();
	OnTargetChanged.Broadcast(nullptr);
}

bool UPDCombatComponent::HasValidTarget() const
{
	AActor* T = CurrentTarget.Get();
	if (!T) return false;

	// 생사 판정은 IPDDamageable 단일 진실 원천 사용
	if (T->Implements<UPDDamageable>())
	{
		return IPDDamageable::Execute_IsAlive(T);
	}
	return true;
}

bool UPDCombatComponent::CanAttack() const
{
	if (IsOnCooldown()) return false;
	if (!HasValidTarget()) return false;

	const AActor* Owner = GetOwner();
	const AActor* Target = CurrentTarget.Get();
	if (!Owner || !Target) return false;

	const float DistSq   = FVector::DistSquared(Owner->GetActorLocation(), Target->GetActorLocation());
	const float Dist     = FMath::Sqrt(DistSq);
	const bool  bInRange = DistSq <= (AttackRange * AttackRange);

#if ENABLE_DRAW_DEBUG
	// pd.ai.debugdraw 1 일 때만 OnScreen 출력.
	if (GEngine)
	{
		const IConsoleVariable* CVar = GetPDAIDebugDrawCVar();
		if (CVar && CVar->GetInt() != 0)
		{
			GEngine->AddOnScreenDebugMessage(
				static_cast<int32>(GetUniqueID()),
				0.3f,
				bInRange ? FColor::Green : FColor::Red,
				FString::Printf(TEXT("[%s] CanAttack: Dist=%.0f / Range=%.0f → %s"),
					*GetNameSafe(Owner), Dist, AttackRange,
					bInRange ? TEXT("YES") : TEXT("NO")));
		}
	}
#else
	(void)Dist;
#endif

	return bInRange;
}

bool UPDCombatComponent::RequestAttack()
{
	if (!CanAttack()) return false;

	UWorld* World = GetWorld();
	if (!World) return false;

	LastAttackTime = World->GetTimeSeconds();
	OnAttackRequested.Broadcast(CurrentTarget.Get());
	return true;
}

bool UPDCombatComponent::IsOnCooldown() const
{
	if (LastAttackTime < 0.f) return false;

	const UWorld* World = GetWorld();
	if (!World) return false;

	return (World->GetTimeSeconds() - LastAttackTime) < AttackCooldown;
}

float UPDCombatComponent::GetCooldownRemaining() const
{
	if (LastAttackTime < 0.f) return 0.f;

	const UWorld* World = GetWorld();
	if (!World) return 0.f;

	const float Elapsed = World->GetTimeSeconds() - LastAttackTime;
	return FMath::Max(0.f, AttackCooldown - Elapsed);
}

void UPDCombatComponent::SetLastNoiseLocation(AActor* NoiseInstigator, const FVector& Location)
{
	LastNoiseInstigator = NoiseInstigator;
	LastNoiseLocation = Location;
	bHasNoiseHint = true;
	OnNoiseHintChanged.Broadcast(Location, true);
}

void UPDCombatComponent::ClearNoiseHint()
{
	if (!bHasNoiseHint) return;
	bHasNoiseHint = false;
	LastNoiseInstigator.Reset();
	OnNoiseHintChanged.Broadcast(LastNoiseLocation, false);
}

FVector UPDCombatComponent::GetFireTraceStart(float ZOffset) const
{
	const AActor* Owner = GetOwner();
	if (!Owner) return FVector::ZeroVector;

	return Owner->GetActorLocation()
		+ Owner->GetActorRightVector() * MuzzleRightOffset
		+ FVector(0.f, 0.f, ZOffset);
}

bool UPDCombatComponent::IsFriendlyInLineOfFire(float ZOffset) const
{
	const AActor* Owner = GetOwner();
	const AActor* Target = CurrentTarget.Get();
	const UWorld* World = GetWorld();
	if (!Owner || !Target || !World) return false;

	uint8 OwnerTeam = FGenericTeamId::NoTeam;
	if (Owner->Implements<UPDCombatInterface>())
	{
		OwnerTeam = IPDCombatInterface::Execute_GetTeamID(Owner);
	}
	// 무팀(NoTeam) 은 우군 개념이 없으므로 검사 자체를 건너뜀.
	if (OwnerTeam == FGenericTeamId::NoTeam) return false;

	// 시작점은 총구 우측 오프셋 반영. 끝점은 target 중심(가슴 Z 동일).
	const FVector Start = GetFireTraceStart(ZOffset);
	const FVector End   = Target->GetActorLocation() + FVector(0.f, 0.f, ZOffset);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(PD_FriendlyLOF), /*bTraceComplex=*/false, Owner);
	Params.AddIgnoredActor(Target);

	FHitResult Hit;
	const bool bBlocked = World->LineTraceSingleByChannel(Hit, Start, End, ECC_Pawn, Params);

	bool bFriendlyHit = false;
	if (bBlocked)
	{
		const AActor* HitActor = Hit.GetActor();
		if (HitActor && HitActor->Implements<UPDCombatInterface>())
		{
			const uint8 HitTeam = IPDCombatInterface::Execute_GetTeamID(HitActor);
			bFriendlyHit = (HitTeam != FGenericTeamId::NoTeam) && (HitTeam == OwnerTeam);
		}
	}

#if ENABLE_DRAW_DEBUG
	if (const IConsoleVariable* CVar = GetPDAIDebugDrawCVar())
	{
		if (CVar->GetInt() != 0)
		{
			// 빨강 = 우군 차단(발사 보류), 주황 = 비-우군 차단, 초록 = 사선 클리어.
			const FColor LineCol  = bFriendlyHit ? FColor::Red : (bBlocked ? FColor::Orange : FColor::Green);
			DrawDebugLine  (World, Start, End,    LineCol,        false, 0.2f, 0, 1.5f);
			DrawDebugSphere(World, Start, 8.f, 8, FColor::Cyan,   false, 0.2f, 0, 1.0f);
			DrawDebugSphere(World, End,   8.f, 8, FColor::Yellow, false, 0.2f, 0, 1.0f);
			if (bFriendlyHit)
			{
				DrawDebugSphere(World, Hit.ImpactPoint, 16.f, 12, FColor::Red, false, 0.3f, 0, 2.f);
			}
		}
	}
#endif

	return bFriendlyHit;
}

void UPDCombatComponent::NotifyAlliesInRadius(float Radius, AActor* SharedTarget)
{
	if (Radius <= 0.f) return;

	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!Owner || !World) return;

	// Cooldown 게이트 — 같은 타겟에 대한 반복 통보 차단.
	// BT 의 Chase 분기가 매 tick 발화되어도 ally 들의 target 이 무한 재설정되지 않도록.
	// 새 타겟이면 cooldown 무시하고 즉시 통보.
	const float Now = World->GetTimeSeconds();
	const bool bSameTargetAsLast = LastNotifiedTarget.IsValid() && LastNotifiedTarget.Get() == SharedTarget;
	if (bSameTargetAsLast && LastNotifyTime > 0.f && (Now - LastNotifyTime) < NotifyAlliesCooldown)
	{
		return;
	}
	LastNotifiedTarget = SharedTarget;
	LastNotifyTime = Now;

	uint8 OwnerTeam = 0;
	if (Owner->Implements<UPDCombatInterface>())
	{
		OwnerTeam = IPDCombatInterface::Execute_GetTeamID(Owner);
	}

	TArray<FOverlapResult> Overlaps;
	const FCollisionShape Sphere = FCollisionShape::MakeSphere(Radius);
	const FCollisionQueryParams Params(SCENE_QUERY_STAT(PD_NotifyAllies), false, Owner);

	const bool bHit = World->OverlapMultiByObjectType(
		Overlaps,
		Owner->GetActorLocation(),
		FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn),
		Sphere,
		Params
	);

	if (!bHit) return;

	for (const FOverlapResult& Result : Overlaps)
	{
		AActor* Other = Result.GetActor();
		if (!Other || Other == Owner) continue;
		if (!Other->Implements<UPDCombatInterface>()) continue;
		if (IPDCombatInterface::Execute_GetTeamID(Other) != OwnerTeam) continue;
		// 생사 판정은 IPDDamageable 사용. 비-Damageable 동맹은 살아있다고 간주.
		if (Other->Implements<UPDDamageable>() && !IPDDamageable::Execute_IsAlive(Other)) continue;

		UPDCombatComponent* AllyCombat = Other->FindComponentByClass<UPDCombatComponent>();
		if (!AllyCombat) continue;
		if (AllyCombat->GetCurrentTarget() == SharedTarget) continue;

		AllyCombat->SetCurrentTarget(SharedTarget);
	}
}
