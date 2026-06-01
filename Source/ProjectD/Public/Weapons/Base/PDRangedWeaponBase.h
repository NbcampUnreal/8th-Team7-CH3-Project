#pragma once

#include "CoreMinimal.h"
#include "Weapons/Base/PDWeaponBase.h"
#include "PDRangedWeaponBase.generated.h"

class UPDInventoryComponent;
class UAbilitySystemComponent;
class USoundBase;
class UParticleSystem;


class UGCN_Weapon_Fire;
class UGCN_Weapon_Impact;
class UGCN_Weapon_ReloadEmpty;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponFired,         APDWeaponBase*, Weapon);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponReloadStarted, APDWeaponBase*, Weapon);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponReloaded,      APDWeaponBase*, Weapon);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FOnWeaponAmmoChanged, APDWeaponBase*, Weapon, int32, CurrentAmmo, int32, MaxAmmo, int32, AvailableAmmo, bool, bIsReloading);

UCLASS(Abstract, Blueprintable)
class PROJECTD_API APDRangedWeaponBase : public APDWeaponBase
{
	GENERATED_BODY()

public:
	APDRangedWeaponBase();

	UPROPERTY(BlueprintAssignable, Category="Weapon")
	FOnWeaponFired OnWeaponFired;

	UPROPERTY(BlueprintAssignable, Category="Weapon")
	FOnWeaponReloadStarted OnWeaponReloadStarted;

	UPROPERTY(BlueprintAssignable, Category="Weapon")
	FOnWeaponReloaded OnWeaponReloaded;

	UPROPERTY(BlueprintAssignable, Category="Weapon")
	FOnWeaponAmmoChanged OnWeaponAmmoChanged;

	bool bCanFire = true;
	FTimerHandle FireCooldownHandle;
	FTimerHandle ReloadHandle;

	virtual void Fire_Implementation()              override;
	virtual void Reload_Implementation()            override;
	virtual void OnEquip_Implementation(AActor* NewOwner) override;
	virtual void OnUnequip_Implementation()         override;

	UFUNCTION(BlueprintPure, Category="Weapon") int32 GetCurrentAmmo()     const { return CurrentAmmo; }
	UFUNCTION(BlueprintPure, Category="Weapon") int32 GetMaxAmmo()         const { return GetCurrentStats().MaxAmmo; }
	UFUNCTION(BlueprintPure, Category="Weapon") bool  IsReloading()        const { return bIsReloading; }
	UFUNCTION(BlueprintPure, Category="Weapon") bool  CanFire()            const;
	UFUNCTION(BlueprintPure, Category="Weapon") bool  CanReload()          const;

	/** 외부에서 영속화된 잔탄을 복원할 때 사용. [0, MaxAmmo]로 클램프. */
	UFUNCTION(BlueprintCallable, Category="Weapon")
	void SetCurrentAmmo(int32 NewAmmo);
	// 자식 클래스가 FireMode(단발/연사) 같은 추가 조건을 더해 override 가능.
	// Player GA_FireAbility / AI Soldier 양쪽 모두 본 메서드 값으로 연사 루프 진입 여부 결정.
	UFUNCTION(BlueprintPure, Category="Weapon") virtual bool IsFullAuto() const { return bFullAuto; }
	UFUNCTION(BlueprintPure, Category="Weapon") int32 GetAvailableAmmoCount() const;
	UFUNCTION(BlueprintPure,     Category="Weapon") bool HasInfiniteAmmo() const   { return bInfiniteAmmo; }
	UFUNCTION(BlueprintCallable, Category="Weapon") void SetInfiniteAmmo(bool bEnabled) { bInfiniteAmmo = bEnabled; }
	UFUNCTION(BlueprintPure, Category="Weapon") int32 GetReserveAmmo() const { return ReserveAmmo; }

	UFUNCTION(BlueprintCallable, Category="Weapon")
	void RefreshAmmoChanged();

	void FinishReload();
	void CancelReload();

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;


	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	bool bFullAuto = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	FName MuzzleSocketName = TEXT("MuzzleSocket");


	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	FName AmmoItemID;





	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|FX")
	TObjectPtr<UParticleSystem> MuzzleFlashEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|FX")
	TObjectPtr<UParticleSystem> TracerParticle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|FX")
	TObjectPtr<UParticleSystem> HitImpactEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|FX")
	TObjectPtr<USoundBase> FireSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|FX")
	TObjectPtr<USoundBase> HitBodySound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|FX")
	TObjectPtr<USoundBase> HitSurfaceSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|FX")
	TObjectPtr<USoundBase> ReloadEmptySound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|FX")
	FName CartridgeEjectSocketName = TEXT("AmmoEject");


	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Animation")
	TObjectPtr<UAnimMontage> FireMontage;


	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Animation")
	TObjectPtr<UAnimMontage> ReloadMontage;



	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Recoil", meta=(ClampMin="0.0"))
	float RecoilYawPerShot = 2.f;



	UPROPERTY(ReplicatedUsing=OnRep_CurrentAmmo, EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|State", meta=(ClampMin="0"))
	int32 CurrentAmmo = 0;

	UPROPERTY(ReplicatedUsing=OnRep_ReserveAmmo, EditAnywhere, BlueprintReadOnly, Category="Weapon|State", meta=(ClampMin="0"))
	int32 ReserveAmmo = 90;

	UPROPERTY(ReplicatedUsing=OnRep_IsReloading, VisibleAnywhere, BlueprintReadOnly, Category="Weapon|State")
	bool bIsReloading = false;

	/** 무한 탄약. true면 장전 모션은 정상 재생되지만 FinishReload에서 인벤토리 무시하고 MaxAmmo로 풀충. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon|State")
	bool bInfiniteAmmo = false;


	bool HasAmmoToReload() const;
	void BroadcastAmmoChanged();
	void ApplyDamage(AActor* HitActor, float DamageAmount, const FHitResult& HitResult);
	void PostFire();
	void ResetFireCooldown();
	void PlayWeaponMontage(UAnimMontage* Montage, FName StartSection = NAME_None);
	bool IsPlayingMontage(UAnimMontage* Montage) const;
	void StopWeaponMontage(UAnimMontage* Montage);
	void BindMontageEndedForReload(UAnimMontage* Montage);
	void ApplyRecoil();


	void ExecuteFireCue(const FVector& MuzzleLoc, const FVector& TraceEnd);


	void ExecuteImpactCue(const FHitResult& Hit);
	void ExecuteReloadCue(float ReloadDuration);
	void ExecuteReloadEmptyCue();
	float GetReloadDuration() const;

	APlayerController*             GetOwnerPlayerController() const;
	UPDInventoryComponent*         GetOwnerInventory()        const;
	class UAbilitySystemComponent* GetOwnerASC()              const;

private:
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastOnWeaponFired();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastOnWeaponReloadStarted();

	UFUNCTION()
	void OnReloadMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UFUNCTION()
	void OnRep_CurrentAmmo();

	UFUNCTION()
	void OnRep_ReserveAmmo();

	UFUNCTION()
	void OnRep_IsReloading();

	friend class UGCN_Weapon_Fire;
	friend class UGCN_Weapon_Impact;
	friend class UGCN_Weapon_ReloadEmpty;
};
