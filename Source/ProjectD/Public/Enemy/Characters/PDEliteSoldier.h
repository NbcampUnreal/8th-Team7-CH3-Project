#pragma once

#include "CoreMinimal.h"
#include "Enemy/Characters/PDSoldier.h"
#include "PDEliteSoldier.generated.h"

class APDCoverBase;

/**
 * 엘리트 솔저.
 *  - APDSoldier 상속하되 부모의 자동 풀오토 발사는 비활성(bAutoFireOnAttackRequested=false).
 *  - 발사 타이밍은 전적으로 BT 가 SetPeeking(true/false) 토글로 제어 — 피크 시에만 사격.
 *  - 커버 점유 라이프사이클(CurrentCover) 을 본 클래스가 단일 진실 원천으로 보관.
 *
 * BT 가 호출하는 API:
 *  - SetInCover(Cover) / SetInCover(nullptr)  — 점유 진입/해제 + 피크 강제 종료.
 *  - SetPeeking(true) / SetPeeking(false)     — 발사 루프 on/off + BP 애니메이션 훅.
 */
UCLASS(Blueprintable)
class PROJECTD_API APDEliteSoldier : public APDSoldier
{
	GENERATED_BODY()

public:
	APDEliteSoldier();

	UFUNCTION(BlueprintPure, Category = "PD|Elite|Cover")
	FORCEINLINE APDCoverBase* GetCurrentCover() const { return CurrentCover.Get(); }

	UFUNCTION(BlueprintPure, Category = "PD|Elite|Cover")
	FORCEINLINE bool IsInCover() const { return bIsInCover; }

	UFUNCTION(BlueprintPure, Category = "PD|Elite|Cover")
	FORCEINLINE bool IsPeeking() const { return bIsPeeking; }

	/** nullptr 전달 시 현재 cover 해제. 같은 cover 재호출은 no-op. 피크 중이었다면 자동 종료. */
	UFUNCTION(BlueprintCallable, Category = "PD|Elite|Cover")
	void SetInCover(APDCoverBase* NewCover);

	/** true → 발사 루프 시작 + BP_OnStartPeek. false → 정지 + BP_OnEndPeek. */
	UFUNCTION(BlueprintCallable, Category = "PD|Elite|Cover")
	void SetPeeking(bool bPeek);

protected:
	virtual void OnEnterState_Dead() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Elite|Cover")
	bool bIsInCover = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Elite|Cover")
	bool bIsPeeking = false;

	UPROPERTY(Transient)
	TWeakObjectPtr<APDCoverBase> CurrentCover;

	// 디자이너용 애니메이션/VFX 훅
	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Elite|Cover")
	void BP_OnEnterCover(APDCoverBase* Cover);

	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Elite|Cover")
	void BP_OnExitCover();

	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Elite|Cover")
	void BP_OnStartPeek();

	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Elite|Cover")
	void BP_OnEndPeek();
};
