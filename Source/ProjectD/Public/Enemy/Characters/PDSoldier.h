#pragma once

#include "CoreMinimal.h"
#include "Enemy/Characters/PDBipedEnemy.h"
#include "GameplayTagContainer.h"
#include "PDSoldier.generated.h"

class APDWeaponBase;
class UAnimInstance;
class UAnimMontage;

/**
 * 일반 보병 적 (Soldier).
 *  - APDBipedEnemy 의 가장 단순한 구체 클래스.
 *  - BeginPlay 에서 DefaultWeaponClass 스폰 + WeaponSocket 부착 + OnEquip(self).
 *
 * 발사 경로 두 갈래 — 무기의 IsFullAuto() 가 분기 결정 (플레이어 GA_FireAbility 와 동일 정책):
 *  1) 연사(Rifle Auto): OnTargetChanged → 무기 FireRate 간격으로 timer 루프. OnFireTick 이 발사.
 *  2) 단발(Rifle Single / Shotgun / Sniper): BT 의 Combat->RequestAttack() → OnAttackRequested → 1회 발사.
 *
 *  - 무기 교체 시 SetEquippedWeapon 에서 timer 재평가 — Rifle↔Sniper 등 전환 즉시 반영.
 *  - bAutoFireOnAttackRequested=false 면 두 경로 모두 차단 — BP 측에서 발사 정책 자유 구현.
 */
UCLASS(Blueprintable)
class PROJECTD_API APDSoldier : public APDBipedEnemy
{
	GENERATED_BODY()

public:
	APDSoldier();

	UFUNCTION(BlueprintPure, Category = "PD|Soldier|Weapon")
	FORCEINLINE APDWeaponBase* GetEquippedWeapon() const { return EquippedWeapon; }

	// PDCharacterBase 공용 인터페이스: AnimInstance 등 상위 코드가 캐릭터 종류와 무관하게 무기 조회.
	virtual APDWeaponBase* GetCurrentWeapon() const override { return EquippedWeapon; }

	// 무기 드랍 — 사망 시 EnemyBase 가 WeaponDropChance 굴려 시체 Stash 에 이전.
	virtual FName GetEquippedWeaponItemID_Implementation() const override;

	/** 런타임 무기 교체. 기존 무기는 OnUnequip 후 Destroy. */
	UFUNCTION(BlueprintCallable, Category = "PD|Soldier|Weapon")
	void SetEquippedWeapon(APDWeaponBase* NewWeapon, bool bDestroyPrevious = true);

protected:
	virtual void BeginPlay() override;
	virtual void OnEnterState_Dead() override;

	/** 디자이너가 BP 디폴트에서 지정. nullptr 이면 무기 미장착 — 발사 시 경고. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Soldier|Weapon")
	TSubclassOf<APDWeaponBase> DefaultWeaponClass;

	/** 타겟 획득 시 자동으로 풀오토 발사 루프 시작 여부. false 면 BP 가 직접 발사 제어. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Soldier|Weapon")
	bool bAutoFireOnAttackRequested = true;

	/** 풀오토 발사 간격(초) 폴백값. 우선순위: 무기 stat 의 FireRate > 본 값. 무기 stat 가 0/미설정일 때만 사용. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Soldier|Weapon",
		meta = (ClampMin = "0.0", ToolTip = "Rifle Auto timer 폴백. 무기 stat FireRate 가 설정돼 있으면 그 값이 우선."))
	float FireInterval = 0.1f;

	/** true 면 사거리 밖에서는 발사를 건너뜀(루프는 유지, 사거리 복귀 시 즉시 재개). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Soldier|Weapon")
	bool bRequireInRangeToFire = true;

	/** 장착 무기에 무한탄약 강제. 장전 모션은 정상 재생되지만 탄은 절대 마르지 않음. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Soldier|Weapon")
	bool bForceInfiniteAmmo = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Soldier|Weapon")
	TObjectPtr<APDWeaponBase> EquippedWeapon;

	/** 무기 미장착 또는 무기에 AnimLayer 가 없을 때 사용할 기본 레이어. */
	UPROPERTY(EditDefaultsOnly, Category = "PD|Soldier|Animation")
	TSubclassOf<UAnimInstance> DefaultAnimLayerClass;

	// 자식 클래스(예: APDEliteSoldier) 가 BT/상태머신 측에서 직접 발사 루프 on/off 하기 위함.
	void StartContinuousFire();
	void StopContinuousFire();

private:
	UFUNCTION()
	void HandleTargetChanged(AActor* NewTarget);

	// BT 의 Combat->RequestAttack() 신호로 단발/비-Rifle 무기를 1회 발사.
	UFUNCTION()
	void HandleAttackRequested(AActor* Target);

	UFUNCTION()
	void OnFireTick();

	void SpawnAndEquipDefaultWeapon();

	void OnWeaponTypeTagChanged(const FGameplayTag Tag, int32 NewCount);
	void LinkDefaultAnimLayer();

	// 현재 장착 무기가 연사 모드인지 — Rifle 이면서 FireMode==Auto.
	// 비-Rifle 또는 Rifle Single 이면 false → BT 의 RequestAttack 으로 단발 발사 경로 사용.
	bool IsCurrentWeaponFullAutoMode() const;

	// HandleAttackRequested 의 공통 발사 시퀀스 (장전/우군 사선 등). 성공 시 true.
	bool TryFireSingleShot();

	FTimerHandle FireTimerHandle;
};
