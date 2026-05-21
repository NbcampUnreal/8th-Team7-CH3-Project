#pragma once

#include "CoreMinimal.h"
#include "Enemy/Characters/PDSoldier.h"
#include "PDEliteSoldier.generated.h"

/**
 * 엘리트 솔저.
 *  - APDSoldier 상속하되 부모의 자동 풀오토 발사는 비활성(bAutoFireOnAttackRequested=false).
 *  - 발사 타이밍은 전적으로 BT 가 SetPeeking(true/false) 토글로 제어 — 피크 시에만 사격.
 *  - Cover 위치는 EQS 가 동적으로 산출, BT 가 MoveTo 로 이동.
 *    본 클래스는 점유 상태(bIsInCover) 만 들고 있음 — APDCoverBase 의존 없음.
 *
 * BT 가 호출하는 API:
 *  - SetInCover(true/false)   — 점유 진입/해제 + 피크 강제 종료.
 *  - SetPeeking(true/false)   — 발사 루프 on/off + BP 애니메이션 훅.
 */
UCLASS(Blueprintable)
class PROJECTD_API APDEliteSoldier : public APDSoldier
{
	GENERATED_BODY()

public:
	APDEliteSoldier();

	UFUNCTION(BlueprintPure, Category = "PD|Elite|Cover")
	FORCEINLINE bool IsInCover() const { return bIsInCover; }

	UFUNCTION(BlueprintPure, Category = "PD|Elite|Cover")
	FORCEINLINE bool IsPeeking() const { return bIsPeeking; }

	/** true → 진입(BP_OnEnterCover). false → 이탈(BP_OnExitCover) + 피크 강제 종료. */
	UFUNCTION(BlueprintCallable, Category = "PD|Elite|Cover")
	void SetInCover(bool bEnter);

	/** true → 발사 루프 시작 + BP_OnStartPeek. false → 정지 + BP_OnEndPeek. */
	UFUNCTION(BlueprintCallable, Category = "PD|Elite|Cover")
	void SetPeeking(bool bPeek);

protected:
	virtual void OnEnterState_Dead() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Elite|Cover")
	bool bIsInCover = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Elite|Cover")
	bool bIsPeeking = false;

	// 디자이너용 애니메이션/VFX 훅. 위치 필요 시 BP 에서 GetActorLocation 으로 조회.
	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Elite|Cover")
	void BP_OnEnterCover();

	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Elite|Cover")
	void BP_OnExitCover();

	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Elite|Cover")
	void BP_OnStartPeek();

	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Elite|Cover")
	void BP_OnEndPeek();
};
