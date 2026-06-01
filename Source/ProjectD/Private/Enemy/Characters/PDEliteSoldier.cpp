#include "Enemy/Characters/PDEliteSoldier.h"

#include "Engine/World.h"
#include "Weapons/Base/PDRangedWeaponBase.h"

APDEliteSoldier::APDEliteSoldier()
{
	// 풀오토 자동 점화는 비활성 — 발사는 SetPeeking 으로만.
	bAutoFireOnAttackRequested = false;
}

void APDEliteSoldier::SetInCover(bool bEnter)
{
	if (bIsInCover == bEnter) return;

	// 이탈 시 피크 중이었다면 먼저 종료(발사 루프 정지 보장).
	if (!bEnter && bIsPeeking)
	{
		SetPeeking(false);
	}

	bIsInCover = bEnter;

	if (bEnter)
	{
		BP_OnEnterCover();
	}
	else
	{
		BP_OnExitCover();
	}
}

void APDEliteSoldier::SetPeeking(bool bPeek)
{
	if (bIsPeeking == bPeek) return;
	bIsPeeking = bPeek;

	if (bPeek)
	{
		StartContinuousFire();
		BP_OnStartPeek();
	}
	else
	{
		StopContinuousFire();
		BP_OnEndPeek();
	}
}

void APDEliteSoldier::OnEnterState_Dead()
{
	// 부모(APDSoldier::OnEnterState_Dead) 가 fire 루프 정지 + 무기 정리 수행 전 cover 상태 정리.
	if (bIsInCover)
	{
		SetInCover(false);
	}

	Super::OnEnterState_Dead();
}

void APDEliteSoldier::SwitchWeaponByDistance(float DistanceToTarget)
{
	const TSubclassOf<APDRangedWeaponBase> Desired = ResolveDesiredWeaponClass(DistanceToTarget);
	if (!Desired) return;

	// 현재 무기 클래스가 desired 와 같으면 swap 불필요.
	const APDWeaponBase* Current = GetEquippedWeapon();
	if (Current && Current->GetClass() == Desired.Get()) return;

	// 사망/전이 중에는 swap 금지.
	if (GetEnemyState() == EPDEnemyState::Dead) return;

	SpawnAndEquipRanged(Desired);
}

TSubclassOf<APDRangedWeaponBase> APDEliteSoldier::ResolveDesiredWeaponClass(float DistanceToTarget) const
{
	// Hysteresis: 현재 무기가 임계값 안쪽 버퍼에 있으면 유지. ping-pong 방지.
	const APDRangedWeaponBase* Cur = Cast<APDRangedWeaponBase>(GetEquippedWeapon());
	const UClass* CurCls = Cur ? Cur->GetClass() : nullptr;

	const float SniperLo = SniperRangeMin - SwapHysteresis;
	const float SniperHi = SniperRangeMin + SwapHysteresis;
	const float RifleLo  = RifleRangeMin  - SwapHysteresis;
	const float RifleHi  = RifleRangeMin  + SwapHysteresis;

	// 임계 버퍼 안에서는 현재 무기가 유효한 후보 중 하나면 유지.
	if (Cur && DistanceToTarget >= SniperLo && DistanceToTarget < SniperHi)
	{
		if (CurCls == SniperWeaponClass.Get() || CurCls == RifleWeaponClass.Get()) return Cur->GetClass();
	}
	if (Cur && DistanceToTarget >= RifleLo && DistanceToTarget < RifleHi)
	{
		if (CurCls == RifleWeaponClass.Get() || CurCls == ShotgunWeaponClass.Get()) return Cur->GetClass();
	}

	// 기본 분류.
	if (DistanceToTarget >= SniperRangeMin) return SniperWeaponClass;
	if (DistanceToTarget >= RifleRangeMin)  return RifleWeaponClass;
	return ShotgunWeaponClass;
}

void APDEliteSoldier::SpawnAndEquipRanged(TSubclassOf<APDRangedWeaponBase> WeaponClass)
{
	// 무기는 서버 권위 — 클라 중복 스폰 방지. 클라는 복제로 받아 OnRep 연출(부모 APDSoldier).
	if (!HasAuthority()) return;
	if (!WeaponClass) return;

	UWorld* World = GetWorld();
	if (!World) return;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	APDRangedWeaponBase* NewWeapon = World->SpawnActor<APDRangedWeaponBase>(
		WeaponClass,
		GetActorLocation(),
		GetActorRotation(),
		SpawnParams);

	if (!NewWeapon) return;

	// 부모(APDSoldier) 가 anim layer 교체 / ASC 태그 / fire timer 재평가 + 이전 무기 Destroy 까지 일괄 처리.
	SetEquippedWeapon(NewWeapon, /*bDestroyPrevious=*/true);
}
