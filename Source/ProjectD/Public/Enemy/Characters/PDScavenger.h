#pragma once

#include "CoreMinimal.h"
#include "Enemy/Characters/PDBipedEnemy.h"
#include "GameplayTagContainer.h"
#include "PDScavenger.generated.h"

class APDWeaponBase;
class UAnimInstance;
class UAnimMontage;
class UGameplayAbility;

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

	// 무기 드랍 — 사망 시 EnemyBase 가 WeaponDropChance 굴려 시체 Stash 에 이전.
	virtual FName GetEquippedWeaponItemID_Implementation() const override;

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

	/**
	 * 휘두름 ability. 몽타주/섹션/sweep/데미지 인가 모두 ability 내부에서 처리.
	 * 디자이너는 GA_PDMeleeAttack 의 BP 자식(예: GA_PDMeleeAttack_Scavenger)을 만들어 몽타주를 지정 후 여기에 할당.
	 * 또한 BP_PDScavenger 의 ActiveAbilities 배열에도 같은 클래스를 추가해야 ASC 가 GiveAbility 한다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Scavenger|Combat")
	TSubclassOf<UGameplayAbility> MeleeAttackAbilityClass;

private:
	UFUNCTION()
	void HandleAttackRequested(AActor* Target);

	void SpawnAndEquipDefaultWeapon();

	void OnWeaponTypeTagChanged(const FGameplayTag Tag, int32 NewCount);
	void LinkDefaultAnimLayer();
};
