#pragma once

#include "CoreMinimal.h"
#include "Weapons/Base/PDRangedWeaponBase.h"
#include "Weapons/PDProjectile.h"
#include "PDSniper.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnScopeToggled, bool, bIsScoped);

UCLASS(Blueprintable)
class PROJECTD_API APDSniper : public APDRangedWeaponBase
{
	GENERATED_BODY()

public:
    APDSniper();

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
    TSubclassOf<APDProjectile> ProjectileClass;

    /** 볼트 액션 (무기 메시) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Animation")
    TObjectPtr<UAnimMontage> BoltActionMontage;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
    TArray<bool> PenetrationPerLevel = { false, false, true };

    // ── 줌 ───────────────────────────────────────────────────────────────────
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Zoom")
    float DefaultFOV = 90.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Zoom")
    float ZoomedFOV = 40.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon|Zoom")
    bool bIsZoomed = false;

public:
    UPROPERTY(BlueprintAssignable, Category="Weapon")
    FOnScopeToggled OnScopeToggled;

    UFUNCTION(BlueprintCallable, Category="Weapon")
    void ToggleZoom();

    UFUNCTION(BlueprintPure, Category="Weapon")
    FORCEINLINE bool IsZoomed() const { return bIsZoomed; }

    virtual void Fire_Implementation() override;

private:
    bool CanPenetrate() const;
    bool BuildAimShot(FVector& OutStart, FVector& OutDirection, FVector& OutTraceEnd) const;
    void SpawnProjectile(bool bPenetrate, const FVector& Start, const FVector& AimDirection);

    UFUNCTION()
    void OnBoltActionMontageEnded(UAnimMontage* Montage, bool bInterrupted);
};
