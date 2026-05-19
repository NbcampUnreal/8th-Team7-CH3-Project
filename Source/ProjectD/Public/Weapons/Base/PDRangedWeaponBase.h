#pragma once

#include "CoreMinimal.h"
#include "Weapons/Base/PDWeaponBase.h"
#include "PDRangedWeaponBase.generated.h"

class UPDInventoryComponent;
class APDCartridge;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponFired,         APDWeaponBase*, Weapon);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponReloadStarted, APDWeaponBase*, Weapon);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponReloaded,      APDWeaponBase*, Weapon);

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

	bool bCanFire = true;
	FTimerHandle FireCooldownHandle;
	FTimerHandle ReloadHandle;

	virtual void Fire_Implementation()              override;
	virtual void Reload_Implementation()            override;
	virtual void OnEquip_Implementation(AActor* NewOwner) override;
	virtual void OnUnequip_Implementation()         override;

	UFUNCTION(BlueprintPure, Category="Weapon") int32 GetCurrentAmmo()     const { return CurrentAmmo; }
	UFUNCTION(BlueprintPure, Category="Weapon") bool  IsReloading()        const { return bIsReloading; }
	UFUNCTION(BlueprintPure, Category="Weapon") bool  CanFire()            const;
	// 자식 클래스가 FireMode(단발/연사) 같은 추가 조건을 더해 override 가능.
	// Player GA_FireAbility / AI Soldier 양쪽 모두 본 메서드 값으로 연사 루프 진입 여부 결정.
	UFUNCTION(BlueprintPure, Category="Weapon") virtual bool IsFullAuto() const { return bFullAuto; }
	UFUNCTION(BlueprintPure, Category="Weapon") int32 GetAvailableAmmoCount() const;
	UFUNCTION(BlueprintPure,     Category="Weapon") bool HasInfiniteAmmo() const   { return bInfiniteAmmo; }
	UFUNCTION(BlueprintCallable, Category="Weapon") void SetInfiniteAmmo(bool bEnabled) { bInfiniteAmmo = bEnabled; }

	void FinishReload();
	void CancelReload();

protected:
	virtual void BeginPlay() override;

	// ── 기본 설정 ─────────────────────────────────────────────────────────────
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	bool bFullAuto = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	FName MuzzleSocketName = TEXT("MuzzleSocket");

	/** 인벤토리에서 소모할 탄약 아이템 ID. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	FName AmmoItemID;


	// ── 이펙트 & 사운드 ───────────────────────────────────────────────────────
	/** 총알이 날아가는 궤적 (Cascade Beam emitter, Target 파라미터로 끝점 지정) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|FX")
	TObjectPtr<UParticleSystem> BeamParticle;

	/** 움직이는 총알 스트릭 (Cascade, SpawnEmitterAtLocation) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|FX")
	TObjectPtr<UParticleSystem> TracerEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|FX")
	TObjectPtr<UParticleSystem> MuzzleFlashEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|FX")
	TObjectPtr<UParticleSystem> HitImpactEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|FX")
	TObjectPtr<USoundBase> FireSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|FX")
	TObjectPtr<USoundBase> HitBodySound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|FX")
	TObjectPtr<USoundBase> HitSurfaceSound;

	/** 탄피 액터 클래스. BP_PDCartridge 할당. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|FX")
	TSubclassOf<APDCartridge> CartridgeClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|FX")
	FName CartridgeEjectSocketName = TEXT("AmmoEject");

	// ── 애니메이션 (무기 메시 전용) ──────────────────────────────────────────
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Animation")
	TObjectPtr<UAnimMontage> FireMontage;

	/** 장전 (무기 메시) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Animation")
	TObjectPtr<UAnimMontage> ReloadMontage;

	// ── 반동 ──────────────────────────────────────────────────────────────────
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Recoil", meta=(ClampMin="0.0"))
	float RecoilYawPerShot = 2.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Recoil", meta=(ClampMin="0.0"))
	float MaxRecoilYaw = 15.f;

	// ── 청각 자극 (AI Perception) ─────────────────────────────────────────────
	/** 총성 청각 자극 반경(cm). 0 이하면 noise 미발생. 발자국(800)보다 훨씬 크게. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Noise", meta=(ClampMin="0.0"))
	float GunshotNoiseRange = 2500.f;

	/** Hearing 자극 태그 — 디자이너가 자극 종류 분류용 (총성/발자국/폭발 등). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Noise")
	FName GunshotNoiseTag = TEXT("Gunshot");

	// ── 런타임 상태 ───────────────────────────────────────────────────────────
	/** 초기 탄 수. 0이면 MaxAmmo(LevelStats)로 가득 채움. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|State", meta=(ClampMin="0"))
	int32 CurrentAmmo = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon|State")
	bool bIsReloading = false;

	/** 무한 탄약. true면 장전 모션은 정상 재생되지만 FinishReload에서 인벤토리 무시하고 MaxAmmo로 풀충. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon|State")
	bool bInfiniteAmmo = false;

	// ── 내부 헬퍼 ────────────────────────────────────────────────────────────
	bool HasAmmoToReload() const;
	void ApplyDamage(AActor* HitActor, float DamageAmount);
	void PostFire();
	void ResetFireCooldown();
	void SpawnBeamEffect(const FVector& Start, const FVector& End);
	void SpawnTracerEffect(const FVector& Start, const FVector& End);
	void PlayWeaponMontage(UAnimMontage* Montage, FName StartSection = NAME_None);
	bool IsPlayingMontage(UAnimMontage* Montage) const;
	void StopWeaponMontage(UAnimMontage* Montage);
	void BindMontageEndedForReload(UAnimMontage* Montage);
	void PlayFireEffects();
	void ApplyRecoil();
	void SpawnImpactEffect(const FHitResult& Hit);
	void SpawnCartridge();
	void PlayHitSound(const FHitResult& Hit);

	APlayerController*     GetOwnerPlayerController() const;
	UPDInventoryComponent* GetOwnerInventory()        const;

private:
	UFUNCTION()
	void OnReloadMontageEnded(UAnimMontage* Montage, bool bInterrupted);
};
