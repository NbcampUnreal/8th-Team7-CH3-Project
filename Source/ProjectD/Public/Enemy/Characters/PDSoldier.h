#pragma once

#include "CoreMinimal.h"
#include "Enemy/Characters/PDBipedEnemy.h"
#include "PDSoldier.generated.h"

class APDWeaponBase;
class UAnimMontage;

/**
 * 일반 보병 적 (Soldier).
 *  - APDBipedEnemy 의 가장 단순한 구체 클래스.
 *  - BeginPlay 에서 DefaultWeaponClass 스폰 + WeaponSocket 부착 + OnEquip(self).
 *  - CombatComponent.OnTargetChanged → 타겟 획득 시 FireInterval 간격으로 풀오토 연사 루프 시작/종료.
 *  - 보스/특수적은 본 클래스 미상속, APDBipedEnemy 또는 APDEnemyBase 직접 상속해서 발사 정책 자유 선택.
 *
 * 확장 포인트:
 *  - 무기 교체: SetEquippedWeapon() 으로 런타임에서 무기 변경 가능.
 *  - 발사 페이스: FireInterval 조절 (무기 단의 자체 쿨다운이 있다면 그쪽이 상한선).
 *  - 발사 직접 제어 끄기: bAutoFireOnAttackRequested=false 후 BP 측에서 OnTargetChanged/OnAttackRequested 처리.
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

	/** 풀오토 발사 간격(초). 무기 단 자체 쿨다운이 더 길면 그쪽이 실효 상한. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Soldier|Weapon",
		meta = (ClampMin = "0.0", ToolTip = "풀오토 시 한 발 사이 간격. 0.1 = 600RPM."))
	float FireInterval = 0.1f;

	/** true 면 사거리 밖에서는 발사를 건너뜀(루프는 유지, 사거리 복귀 시 즉시 재개). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Soldier|Weapon")
	bool bRequireInRangeToFire = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Soldier|Weapon")
	TObjectPtr<APDWeaponBase> EquippedWeapon;

	// 자식 클래스(예: APDEliteSoldier) 가 BT/상태머신 측에서 직접 발사 루프 on/off 하기 위함.
	void StartContinuousFire();
	void StopContinuousFire();

private:
	UFUNCTION()
	void HandleTargetChanged(AActor* NewTarget);

	UFUNCTION()
	void OnFireTick();

	void SpawnAndEquipDefaultWeapon();

	FTimerHandle FireTimerHandle;
};
