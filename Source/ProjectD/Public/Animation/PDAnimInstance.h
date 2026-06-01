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


USTRUCT(BlueprintType)
struct FPDWeaponAnimSet
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> EquipMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FName EquipStartSection = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> FireMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> ReloadMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FName ReloadStartSection = NAME_None;
};

USTRUCT()
struct FPDAnimInstanceCache
{
	GENERATED_BODY()

	float MovementSpeed=0.f;
	float Direction=0.f;
	bool bIsAcc=false;
	bool bIsAiming=false;
	bool bIsJumping=false;
	float AimYaw=0.f;
	float AimPitch=0.f;
	float YawDeltaA=0.f;

	EWeaponType WeaponType=EWeaponType::None;

	FVector LeftHandIKTarget=FVector::ZeroVector;
	float  LeftHandIKAlpha=0.f;

	bool bIsMeleeEquipped=false;
	bool bIsDowned=false;
	bool bIsSprinting=false;
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

	UPROPERTY(BlueprintReadOnly, Category="Animation", meta=(DisplayName="Is Acc"))
	bool bIsAcc=false;

	UPROPERTY(BlueprintReadOnly, Category="Animation")
	bool bIsAiming=false;

	UPROPERTY(BlueprintReadOnly, Category="Animation")
	bool bIsJumping=false;

	UPROPERTY(BlueprintReadOnly, Category="Animation")
	float AimYaw=0.f;

	UPROPERTY(BlueprintReadOnly, Category="Animation")
	float AimPitch=0.f;

	UPROPERTY(BlueprintReadOnly, Category="Animation", meta=(DisplayName="Yaw Delta A"))
	float YawDeltaA=0.f;

	UPROPERTY(BlueprintReadOnly, Category="Animation")
	EWeaponType WeaponType=EWeaponType::None;

	UPROPERTY(BlueprintReadOnly, Category="Animation|IK")
	FVector LeftHandIKTarget=FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category="Animation|IK")
	float LeftHandIKAlpha=0.f;

	UPROPERTY(BlueprintReadOnly, Category="Animation")
	bool bIsMeleeEquipped=false;

	UPROPERTY(BlueprintReadOnly, Category="Animation")
	bool bIsDowned=false;

	UPROPERTY(BlueprintReadOnly, Category="Animation")
	bool bIsSprinting=false;


	UPROPERTY(EditDefaultsOnly, Category="Animation|Weapon|Rifle")
	FPDWeaponAnimSet RifleAnimSet;

	UPROPERTY(EditDefaultsOnly, Category="Animation|Weapon|Shotgun")
	FPDWeaponAnimSet ShotgunAnimSet;

	UPROPERTY(EditDefaultsOnly, Category="Animation|Weapon|Sniper")
	FPDWeaponAnimSet SniperAnimSet;

	UPROPERTY(EditDefaultsOnly, Category="Animation|Weapon|Melee")
	FPDWeaponAnimSet MeleeAnimSet;


	UPROPERTY(EditDefaultsOnly, Category="Animation|HitReact")
	TObjectPtr<UAnimMontage> HitMontage_1;

	UPROPERTY(EditDefaultsOnly, Category="Animation|HitReact")
	TObjectPtr<UAnimMontage> HitMontage_2;

	UPROPERTY(EditDefaultsOnly, Category="Animation|HitReact")
	TObjectPtr<UAnimMontage> HitMontage_3;

	UPROPERTY(EditDefaultsOnly, Category="Animation|HitReact")
	TObjectPtr<UAnimMontage> HitMontage_4;


	UFUNCTION(BlueprintCallable, Category="Animation|HitReact")
	void PlayHitReaction();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animation|Downed")
	TObjectPtr<UAnimMontage> GetUpMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animation|Downed", meta=(ClampMin="0.0", ForceUnits="s"))
	float GetUpDuration = 0.f;

	UFUNCTION(BlueprintCallable, Category="Animation|Downed")
	void PlayGetUpMontage();

	UFUNCTION(BlueprintPure, Category="Animation|Downed")
	float GetGetUpMontageDuration() const;

	UFUNCTION(BlueprintCallable, Category="Animation|Weapon")
	void OnWeaponEquipped(APDRangedWeaponBase* Weapon);


	UFUNCTION(BlueprintCallable, Category="Animation|Weapon")
	void OnWeaponUnequipped(APDRangedWeaponBase* Weapon);

	float GetReloadMontageDurationForWeapon(APDWeaponBase* Weapon) const;
	void StopReloadMontageForWeapon(APDWeaponBase* Weapon, float BlendOutTime = 0.15f);

private:
	UPROPERTY()
	TObjectPtr<APDCharacterBase> OwnerCharacter;

	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> CachedASC;

	FPDAnimInstanceCache Cache;

	UPROPERTY()
	TWeakObjectPtr<APDRangedWeaponBase> BoundWeapon;

	float PreviousActorYaw=0.f;
	bool bHasPreviousActorYaw=false;

	const FPDWeaponAnimSet* GetAnimSetForWeapon(APDWeaponBase* Weapon) const;

	UFUNCTION() void HandleWeaponFired(APDWeaponBase* Weapon);
	UFUNCTION() void HandleWeaponReloadStarted(APDWeaponBase* Weapon);


	void OnEquipMontageEnded(UAnimMontage* Montage, bool bInterrupted);
};
