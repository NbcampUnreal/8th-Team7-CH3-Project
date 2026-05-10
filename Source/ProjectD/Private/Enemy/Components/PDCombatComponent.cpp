#include "Enemy/Components/PDCombatComponent.h"

#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Enemy/Interfaces/PDCombatInterface.h"
#include "Interfaces/PDDamageable.h"

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

	const float DistSq = FVector::DistSquared(Owner->GetActorLocation(), Target->GetActorLocation());
	return DistSq <= (AttackRange * AttackRange);
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

void UPDCombatComponent::NotifyAlliesInRadius(float Radius, AActor* SharedTarget)
{
	if (Radius <= 0.f) return;

	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!Owner || !World) return;

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
