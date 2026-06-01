#pragma once

#include "CoreMinimal.h"
#include "Enemy/Characters/PDSoldier.h"
#include "PDEliteSoldier.generated.h"

class APDRangedWeaponBase;

/**
 * 엘리트 솔저.
 *  - APDSoldier 상속하되 부모의 자동 풀오토 발사는 비활성(bAutoFireOnAttackRequested=false).
 *  - 발사 타이밍은 전적으로 BT 가 SetPeeking(true/false) 토글로 제어 — 피크 시에만 사격.
 *  - Cover 위치는 EQS 가 동적으로 산출, BT 가 MoveTo 로 이동.
 *    본 클래스는 점유 상태(bIsInCover) 만 들고 있음 — APDCoverBase 의존 없음.
 *  - 거리별 무기 자동 교체: 원거리 Sniper / 중거리 Rifle / 근거리 Shotgun.
 *    BT Service(PDBTService_SwitchWeaponByDistance) 가 매 tick SwitchWeaponByDistance 호출.
 *
 * BT 가 호출하는 API:
 *  - SetInCover(true/false)        — 점유 진입/해제 + 피크 강제 종료.
 *  - SetPeeking(true/false)        — 발사 루프 on/off + BP 애니메이션 훅.
 *  - SwitchWeaponByDistance(Dist)  — 거리 임계값 + hysteresis 로 무기 교체.
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

	/** 거리 임계값에 따라 적절한 무기로 교체(이미 그 무기면 no-op). Hysteresis 로 ping-pong 차단. */
	UFUNCTION(BlueprintCallable, Category = "PD|Elite|Weapon")
	void SwitchWeaponByDistance(float DistanceToTarget);

protected:
	virtual void OnEnterState_Dead() override;

	// peek 사격 중에는 무기 타입(풀오토/단발) 무관하게 timer 유지 — Shotgun/Sniper 도 발사 가능.
	virtual bool ShouldForceContinuousFire() const override { return bIsPeeking; }

	/** 거리 ≥ SniperRangeMin 일 때 장착. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Elite|Weapon")
	TSubclassOf<APDRangedWeaponBase> SniperWeaponClass;

	/** SniperRangeMin > 거리 ≥ RifleRangeMin 일 때 장착. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Elite|Weapon")
	TSubclassOf<APDRangedWeaponBase> RifleWeaponClass;

	/** 거리 < RifleRangeMin 일 때 장착. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Elite|Weapon")
	TSubclassOf<APDRangedWeaponBase> ShotgunWeaponClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Elite|Weapon", meta = (ClampMin = "0.0"))
	float SniperRangeMin = 1200.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Elite|Weapon", meta = (ClampMin = "0.0"))
	float RifleRangeMin = 400.f;

	/** 임계값 양쪽 ±버퍼. 임계 근처에서 무기 교체가 반복되는 ping-pong 방지. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Elite|Weapon", meta = (ClampMin = "0.0"))
	float SwapHysteresis = 100.f;

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

private:
	// 현재 거리에 맞는 desired weapon class. 임계값 + hysteresis 적용.
	TSubclassOf<APDRangedWeaponBase> ResolveDesiredWeaponClass(float DistanceToTarget) const;

	// 신규 무기 spawn 후 SetEquippedWeapon(..., bDestroyPrevious=true) 호출.
	void SpawnAndEquipRanged(TSubclassOf<APDRangedWeaponBase> WeaponClass);
};
