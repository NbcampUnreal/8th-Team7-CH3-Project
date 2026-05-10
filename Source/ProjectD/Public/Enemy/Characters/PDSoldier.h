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
 *  - CombatComponent.OnAttackRequested → 장착된 APDWeaponBase->Fire() 호출.
 *  - 보스/특수적은 본 클래스 미상속, APDBipedEnemy 또는 APDEnemyBase 직접 상속해서 발사 정책 자유 선택.
 *
 * 확장 포인트:
 *  - 무기 교체: SetEquippedWeapon() 으로 런타임에서 무기 변경 가능.
 *  - 발사 모션: AttackMontage 로 BP 디자이너가 애님 지정.
 *  - 발사 직접 제어 끄기: bAutoFireOnAttackRequested=false 후 BP 측에서 OnAttackRequested 처리.
 */
UCLASS(Blueprintable)
class PROJECTD_API APDSoldier : public APDBipedEnemy
{
	GENERATED_BODY()

public:
	APDSoldier();

	UFUNCTION(BlueprintPure, Category = "PD|Soldier|Weapon")
	FORCEINLINE APDWeaponBase* GetEquippedWeapon() const { return EquippedWeapon; }

	/** 런타임 무기 교체. 기존 무기는 OnUnequip 후 Destroy. */
	UFUNCTION(BlueprintCallable, Category = "PD|Soldier|Weapon")
	void SetEquippedWeapon(APDWeaponBase* NewWeapon, bool bDestroyPrevious = true);

protected:
	virtual void BeginPlay() override;
	virtual void OnEnterState_Dead() override;

	/** 디자이너가 BP 디폴트에서 지정. nullptr 이면 무기 미장착 — 발사 시 경고. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Soldier|Weapon")
	TSubclassOf<APDWeaponBase> DefaultWeaponClass;

	/** OnAttackRequested 수신 시 자동 Fire 호출 여부. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Soldier|Weapon")
	bool bAutoFireOnAttackRequested = true;

	/** 발사 시 재생할 어택 몽타주 (선택). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Soldier|Weapon")
	TObjectPtr<UAnimMontage> AttackMontage;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Soldier|Weapon")
	TObjectPtr<APDWeaponBase> EquippedWeapon;

private:
	UFUNCTION()
	void HandleAttackRequested(AActor* Target);

	void SpawnAndEquipDefaultWeapon();
};
