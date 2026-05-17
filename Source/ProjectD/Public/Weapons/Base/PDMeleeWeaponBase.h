#pragma once

#include "CoreMinimal.h"
#include "Weapons/Base/PDWeaponBase.h"
#include "PDMeleeWeaponBase.generated.h"

UCLASS(Blueprintable)
class PROJECTD_API APDMeleeWeaponBase : public APDWeaponBase
{
	GENERATED_BODY()

public:
	APDMeleeWeaponBase();

	UFUNCTION(BlueprintPure, Category="Weapon")
	FORCEINLINE float GetSweepRadius()  const { return SweepRadius; }

	UFUNCTION(BlueprintPure, Category="Weapon")
	FORCEINLINE float GetSweepRange()   const { return SweepRange; }

	UFUNCTION(BlueprintPure, Category="Weapon")
	FORCEINLINE FName GetHitSocketName() const { return HitSocketName; }

	UFUNCTION(BlueprintPure, Category="Weapon")
	FORCEINLINE USoundBase* GetSwingSound() const { return SwingSound; }

	UFUNCTION(BlueprintPure, Category="Weapon")
	FORCEINLINE USoundBase* GetHitSound()   const { return HitSound; }

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	float SweepRadius = 30.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	float SweepRange = 80.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	FName HitSocketName = TEXT("weapon_r");

	/** 공격 모션 시작 시 재생 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|FX")
	TObjectPtr<USoundBase> SwingSound;

	/** 적 명중 시 재생 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|FX")
	TObjectPtr<USoundBase> HitSound;
};
