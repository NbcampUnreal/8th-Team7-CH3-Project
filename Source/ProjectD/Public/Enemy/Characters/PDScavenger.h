#pragma once

#include "CoreMinimal.h"
#include "Enemy/Characters/PDBipedEnemy.h"
#include "GameplayTagContainer.h"
#include "PDScavenger.generated.h"

class APDWeaponBase;
class UAnimInstance;
class UAnimMontage;

/**
 * 근접 공격형 적 (Scavenger).
 *  - APDSoldier 와 같은 BipedEnemy 계층의 형제 클래스. 행동(BT/Perception) 은 Soldier 와 동일하게 BP 측에서 구성.
 *  - BeginPlay 에서 DefaultWeaponClass(예: BP_WeaponBat) 스폰 + WeaponSocket 부착 + OnEquip(self).
 *  - CombatComponent.OnAttackRequested → EquippedWeapon->Fire() 위임.
 *    휘두름 몽타주/히트 판정/데미지 인가는 모두 무기 측 책임 (Fire 의 BP 구현).
 *  - 사망 시 OnUnequip + 시체 Stash 이전 (Soldier 와 동일 흐름).
 *
 * 확장 포인트:
 *  - 무기 교체: SetEquippedWeapon() 으로 런타임 변경.
 *  - 발사 직접 제어 끄기: bAutoFireOnAttackRequested=false 후 BP 측 OnAttackRequested 처리.
 */
UCLASS(Blueprintable)
class PROJECTD_API APDScavenger : public APDBipedEnemy
{
	GENERATED_BODY()

public:
	APDScavenger();

	UFUNCTION(BlueprintPure, Category = "PD|Scavenger|Weapon")
	FORCEINLINE APDWeaponBase* GetEquippedWeapon() const { return EquippedWeapon; }

	// PDCharacterBase 공용 인터페이스: AnimInstance 등 상위 코드가 캐릭터 종류와 무관하게 무기 조회.
	virtual APDWeaponBase* GetCurrentWeapon() const override { return EquippedWeapon; }

	/** 런타임 무기 교체. 기존 무기는 OnUnequip 후 Destroy. */
	UFUNCTION(BlueprintCallable, Category = "PD|Scavenger|Weapon")
	void SetEquippedWeapon(APDWeaponBase* NewWeapon, bool bDestroyPrevious = true);

protected:
	virtual void BeginPlay() override;
	virtual void OnEnterState_Dead() override;

	/** 디자이너가 BP 디폴트에서 지정 (예: BP_WeaponBat). nullptr 이면 무기 미장착 — 공격 시 경고. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Scavenger|Weapon")
	TSubclassOf<APDWeaponBase> DefaultWeaponClass;

	/** OnAttackRequested 시 자동으로 EquippedWeapon->Fire() 호출. false 면 BP 가 OnAttackRequested 처리. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Scavenger|Weapon")
	bool bAutoFireOnAttackRequested = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Scavenger|Weapon")
	TObjectPtr<APDWeaponBase> EquippedWeapon;

	/** 무기 미장착 또는 무기에 AnimLayer 가 없을 때 사용할 기본 레이어. */
	UPROPERTY(EditDefaultsOnly, Category = "PD|Scavenger|Animation")
	TSubclassOf<UAnimInstance> DefaultAnimLayerClass;

	/** 공격 시 재생할 캐릭터 측 휘두름 몽타주. AttackSections 와 함께 사용. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Scavenger|Combat")
	TObjectPtr<UAnimMontage> AttackMontage;

	/** AttackMontage 의 섹션 이름 목록. 공격 시 랜덤 한 개 재생. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Scavenger|Combat")
	TArray<FName> AttackSections;

private:
	UFUNCTION()
	void HandleAttackRequested(AActor* Target);

	void SpawnAndEquipDefaultWeapon();

	void OnWeaponTypeTagChanged(const FGameplayTag Tag, int32 NewCount);
	void LinkDefaultAnimLayer();

	/** 휘두름 sweep + 데미지 인가. friendly fire 차단 포함. */
	void PerformMeleeAttack();
};
