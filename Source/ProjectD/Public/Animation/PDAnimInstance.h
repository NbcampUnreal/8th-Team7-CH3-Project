#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Type/Types.h"
#include "GameplayTagContainer.h"
#include "PDAnimInstance.generated.h"

class APDCharacterBase;
class APDWeaponBase;
class APDRangedWeaponBase;
class UAbilitySystemComponent;

/**
 * 무기 한 종류의 캐릭터 애니메이션 세트.
 * AnimBP 클래스 기본값에서 무기 타입별로 직접 할당한다.
 */
USTRUCT(BlueprintType)
struct FPDWeaponAnimSet
{
	GENERATED_BODY()

	/** 무기 꺼낼 때 캐릭터 상체 몽타주 (UpperBody 슬롯) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> EquipMontage;

	/** Equip 몽타주 시작 섹션. None이면 처음부터 재생. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FName EquipStartSection = NAME_None;

	/** 발사 시 캐릭터 상체 몽타주 (UpperBody 슬롯) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> FireMontage;

	/** 장전 시 캐릭터 상체 몽타주 (UpperBody 슬롯) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> ReloadMontage;

	/** Reload 몽타주 시작 섹션. None이면 처음부터 재생. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FName ReloadStartSection = NAME_None;
};

USTRUCT()
struct FPDAnimInstanceCache
{
	GENERATED_BODY()

	float MovementSpeed=0.f;
	float Direction=0.f;
	bool bIsAiming=false;
	bool bIsJumping=false;
	float AimYaw=0.f;
	float AimPitch=0.f;

	EWeaponType WeaponType=EWeaponType::None;

	FVector LeftHandIKTarget=FVector::ZeroVector;
	float  LeftHandIKAlpha=0.f;

	bool bIsInCover=false;
	bool bIsCoverAiming=false;
	bool bIsMeleeEquipped=false;
};

UCLASS()
class PROJECTD_API UPDAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;

	UPROPERTY(BlueprintReadOnly, Category="Animation")
	float MovementSpeed=0.f;

	UPROPERTY(BlueprintReadOnly, Category="Animation")
	float Direction=0.f;

	UPROPERTY(BlueprintReadOnly, Category="Animation")
	bool bIsAiming=false;

	UPROPERTY(BlueprintReadOnly, Category="Animation")
	bool bIsJumping=false;

	UPROPERTY(BlueprintReadOnly, Category="Animation")
	float AimYaw=0.f;

	UPROPERTY(BlueprintReadOnly, Category="Animation")
	float AimPitch=0.f;

	UPROPERTY(BlueprintReadOnly, Category="Animation")
	EWeaponType WeaponType=EWeaponType::None;

	UPROPERTY(BlueprintReadOnly, Category="Animation|IK")
	FVector LeftHandIKTarget=FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category="Animation|IK")
	float LeftHandIKAlpha=0.f;

	UPROPERTY(BlueprintReadOnly, Category="Animation")
	bool bIsInCover=false;

	UPROPERTY(BlueprintReadOnly, Category="Animation")
	bool bIsCoverAiming=false;

	UPROPERTY(BlueprintReadOnly, Category="Animation")
	bool bIsMeleeEquipped=false;

	// ── 무기 타입별 캐릭터 애니메이션 세트 (AnimBP 클래스 기본값에서 할당) ─────
	UPROPERTY(EditDefaultsOnly, Category="Animation|Weapon|Rifle")
	FPDWeaponAnimSet RifleAnimSet;

	UPROPERTY(EditDefaultsOnly, Category="Animation|Weapon|Shotgun")
	FPDWeaponAnimSet ShotgunAnimSet;

	UPROPERTY(EditDefaultsOnly, Category="Animation|Weapon|Sniper")
	FPDWeaponAnimSet SniperAnimSet;

	UPROPERTY(EditDefaultsOnly, Category="Animation|Weapon|Melee")
	FPDWeaponAnimSet MeleeAnimSet;

	// ── 피격 반응 몽타주 (DefaultSlot) — 할당된 것 중 랜덤 재생 ───────────────
	UPROPERTY(EditDefaultsOnly, Category="Animation|HitReact")
	TObjectPtr<UAnimMontage> HitMontage_1;

	UPROPERTY(EditDefaultsOnly, Category="Animation|HitReact")
	TObjectPtr<UAnimMontage> HitMontage_2;

	UPROPERTY(EditDefaultsOnly, Category="Animation|HitReact")
	TObjectPtr<UAnimMontage> HitMontage_3;

	UPROPERTY(EditDefaultsOnly, Category="Animation|HitReact")
	TObjectPtr<UAnimMontage> HitMontage_4;

	/** 할당된 HitMontage 1~4 중 랜덤 하나를 재생 */
	UFUNCTION(BlueprintCallable, Category="Animation|HitReact")
	void PlayHitReaction();

	/**
	 * 무기 장착 시 캐릭터에서 호출.
	 * 델리게이트를 바인딩하고 Equip 몽타주를 재생한다.
	 */
	UFUNCTION(BlueprintCallable, Category="Animation|Weapon")
	void OnWeaponEquipped(APDRangedWeaponBase* Weapon);

	/**
	 * 무기 해제 시 캐릭터에서 호출.
	 * 델리게이트 바인딩을 해제한다.
	 */
	UFUNCTION(BlueprintCallable, Category="Animation|Weapon")
	void OnWeaponUnequipped(APDRangedWeaponBase* Weapon);

private:
	UPROPERTY()
	TObjectPtr<APDCharacterBase> OwnerCharacter;

	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> CachedASC;

	FPDAnimInstanceCache Cache;

	UPROPERTY()
	TWeakObjectPtr<APDRangedWeaponBase> BoundWeapon;

	/** 무기 타입 태그로 AnimSet을 찾아 반환 */
	const FPDWeaponAnimSet* GetAnimSetForWeapon(APDWeaponBase* Weapon) const;

	UFUNCTION() void HandleWeaponFired(APDWeaponBase* Weapon);
	UFUNCTION() void HandleWeaponReloadStarted(APDWeaponBase* Weapon);

	/** Equip 몽타주 종료 시 발사 차단 해제 */
	void OnEquipMontageEnded(UAnimMontage* Montage, bool bInterrupted);
};