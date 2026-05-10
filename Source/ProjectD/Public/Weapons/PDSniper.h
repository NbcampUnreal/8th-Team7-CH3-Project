// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapons/PDWeaponBase.h"
#include "Weapons/PDProjectile.h"
#include "PDSniper.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnScopeToggled, bool, bIsScoped);

UCLASS(Blueprintable)
class PROJECTD_API APDSniper : public APDWeaponBase
{
	GENERATED_BODY()
	
public:
    APDSniper();

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon|Sniper")
    TSubclassOf<APDProjectile> ProjectileClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon|Sniper")
    TArray<bool> PenetrationPerLevel = { false, false, true };

    // 줌 (저격총 전용)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon|Sniper|Zoom")
    float DefaultFOV = 90.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon|Sniper|Zoom")
    float ZoomedFOV = 40.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Weapon|Sniper|Zoom")
    bool bIsZoomed = false;

public:
    UPROPERTY(BlueprintAssignable, Category = "PD|Weapon|Events")
    FOnScopeToggled OnScopeToggled;

    UFUNCTION(BlueprintCallable, Category = "PD|Weapon|Sniper")
    void ToggleZoom();

    UFUNCTION(BlueprintPure, Category = "PD|Weapon|Sniper")
    FORCEINLINE bool IsZoomed() const { return bIsZoomed; }
    
    virtual void Fire_Implementation() override;
    virtual void Reload_Implementation() override;

private:
    bool CanPenetrate() const;
    void SpawnProjectile(bool bPenetrate);
    FVector GetAimDirection() const;

    UFUNCTION()
    void OnBoltActionMontageEnded(UAnimMontage* Montage, bool bInterrupted);
};
