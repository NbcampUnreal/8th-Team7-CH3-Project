#include "Enemy/Characters/PDEliteSoldier.h"

#include "Cover/PDCoverBase.h"

APDEliteSoldier::APDEliteSoldier()
{
	// 풀오토 자동 점화는 비활성 — 발사는 SetPeeking 으로만.
	bAutoFireOnAttackRequested = false;
}

void APDEliteSoldier::SetInCover(APDCoverBase* NewCover)
{
	APDCoverBase* Prev = CurrentCover.Get();
	if (Prev == NewCover) return;

	// 피크 중이었다면 먼저 종료(발사 루프 정지 보장).
	if (bIsPeeking)
	{
		SetPeeking(false);
	}

	if (Prev)
	{
		Prev->Release(this);
	}

	CurrentCover = NewCover;
	bIsInCover = (NewCover != nullptr);

	if (NewCover)
	{
		BP_OnEnterCover(NewCover);
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
	// 부모(APDSoldier::OnEnterState_Dead) 가 fire 루프 정지 + 무기 정리를 수행하기 전 cover 점유 해제.
	if (CurrentCover.IsValid())
	{
		SetInCover(nullptr);
	}

	Super::OnEnterState_Dead();
}
