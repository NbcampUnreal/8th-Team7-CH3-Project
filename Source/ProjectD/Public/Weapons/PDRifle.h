// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapons/Base/PDRangedWeaponBase.h"
#include "Type/Types.h"
#include "PDRifle.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFireModeChanged, EFireMode, NewFireMode);

UCLASS(Blueprintable)
class PROJECTD_API APDRifle : public APDRangedWeaponBase
{
	GENERATED_BODY()

public:
    APDRifle();

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
    EFireMode FireMode = EFireMode::Auto;

public:
    UPROPERTY(BlueprintAssignable, Category = "Weapon")
    FOnFireModeChanged OnFireModeChanged;

    virtual void Fire_Implementation() override;

    // bFullAuto 와 FireMode 두 조건 모두 만족할 때만 true.
    // FireMode==Single 이면 base 의 bFullAuto 가 true 여도 false 반환 → 연사 루프 진입 차단.
    virtual bool IsFullAuto() const override;

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void ToggleFireMode();

    UFUNCTION(BlueprintPure, Category = "Weapon")
    FORCEINLINE EFireMode GetFireMode() const { return FireMode; }

private:
    bool PerformLineTrace(FHitResult& OutHit);
};