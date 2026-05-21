#include "Enemy/Characters/PDEliteSoldier.h"

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
