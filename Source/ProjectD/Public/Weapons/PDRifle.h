// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapons/PDWeaponBase.h"
#include "Type/Types.h"
#include "PDRifle.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFireModeChanged, EFireMode, NewFireMode);

UCLASS(Blueprintable)
class PROJECTD_API APDRifle : public APDWeaponBase
{
	GENERATED_BODY()

public:
    APDRifle();

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon|Rifle")
    EFireMode FireMode = EFireMode::Auto;

    FTimerHandle AutoFireHandle;
    bool bIsFiring = false;

public:
    UPROPERTY(BlueprintAssignable, Category = "PD|Weapon|Events")
    FOnFireModeChanged OnFireModeChanged;

public:
    virtual void Fire_Implementation() override;
    virtual void Reload_Implementation() override;
    virtual void OnUnequip_Implementation() override;

    UFUNCTION(BlueprintCallable, Category = "PD|Weapon|Rifle")
    void StartFire();

    UFUNCTION(BlueprintCallable, Category = "PD|Weapon|Rifle")
    void StopFire();

    UFUNCTION(BlueprintCallable, Category = "PD|Weapon|Rifle")
    void ToggleFireMode();

    UFUNCTION(BlueprintPure, Category = "PD|Weapon|Rifle")
    FORCEINLINE EFireMode GetFireMode() const { return FireMode; }

private:
    bool PerformLineTrace(FHitResult& OutHit);
};