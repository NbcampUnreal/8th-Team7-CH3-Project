#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Type/Types.h"
#include "GameplayTag/PDGameplayTags.h"
#include "Animation/AnimMontage.h"
#include "Interfaces/PDInteractable.h"
#include "Camera/CameraShakeBase.h"
#include "PDWeaponBase.generated.h"

class APDWeaponBase;
class APDShellActor;
class APDMagazineActor;
class USphereComponent;
class UNiagaraSystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponFired, APDWeaponBase*, Weapon);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeaponLevelChanged, APDWeaponBase*, Weapon, int32, NewLevel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponReloaded, APDWeaponBase*, Weapon);

UCLASS(Abstract, Blueprintable)
class PROJECTD_API APDWeaponBase : public AActor, public IPDInteractable
{
	GENERATED_BODY()
	friend class APDPlayerCharacter;

public:
	APDWeaponBase();

protected:
	virtual void BeginPlay() override;

protected:

	// 컴포넌트
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Weapon|Component")
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Weapon|Component")
	TObjectPtr<USphereComponent> PickupCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Weapon|Component")
	TObjectPtr<UStaticMeshComponent> MagazineMesh;

	// 설정
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon|Config")
	EWeaponType WeaponType = EWeaponType::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon|Config")
	FName MuzzleSocketName = TEXT("MuzzleSocket");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon|Stats")
	TArray<FWeaponLevelStats> LevelStats;

	//IK&&Layer
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Weapon|GAS")
	FGameplayTag WeaponTypeTag;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Weapon|Animation")
	TSubclassOf<UAnimInstance> WeaponAnimLayerClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Weapon|Animation")
	FName LeftHandGripSocket=TEXT("LeftHandGrip");
	

	// 애니메이션
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon|Animation")
	TObjectPtr<UAnimMontage> ReloadMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon|Animation")
	TObjectPtr<UAnimMontage> BoltActionMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon|Animation")
	FName EjectionPortSocket = TEXT("EjectionPort");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon|Animation")
	FName MagazineSocketName = TEXT("MagazineSocket");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon|Animation")
	TSubclassOf<APDShellActor> ShellActorClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon|Animation")
	TSubclassOf<APDMagazineActor> MagazineActorClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon|Animation")
	FVector ShellEjectLocalDir = FVector(0.f, 1.f, 0.3f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon|Animation")
	float ShellEjectSpeed = 200.f;

	// 상태
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Weapon|State")
	int32 CurrentLevel = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Weapon|State")
	int32 CurrentAmmo = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Weapon|State")
	bool bIsReloading = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Weapon|State")
	bool bIsDropped = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Weapon")
	TWeakObjectPtr<AActor> WeaponOwner;

	// Recoil 설정
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon|Recoil")
	TSubclassOf<UCameraShakeBase> FireCameraShakeClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon|Recoil",
		meta = (ClampMin = "0.0", ToolTip = "발사마다 추가되는 스프레드 (도)"))
	float RecoilSpreadPerShot = 1.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon|Recoil",
		meta = (ClampMin = "0.0", ToolTip = "최대 스프레드 누적량 (도)"))
	float MaxRecoilSpread = 8.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon|Recoil",
		meta = (ClampMin = "0.0", ToolTip = "초당 회복되는 스프레드 (도)"))
	float RecoilRecoveryRate = 5.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon|Recoil",
		meta = (ToolTip = "발사 시 총 메시 회전  (Pitch = 위로 튀는 정도)"))
	FRotator MeshRecoilKick = FRotator(-4.f, 0.f, 0.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon|Recoil",
		meta = (ClampMin = "1.0", ToolTip = "메시 반동 복구 속도"))
	float MeshRecoilRecoverySpeed = 12.f;

	// 현재 반동 스프레드
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Weapon|Recoil")
	float CurrentRecoilSpread = 0.f;

public:

	bool bCanFire = true;
	FTimerHandle FireCooldownHandle;
	FTimerHandle ReloadHandle;

public:
	
	// 게임플레이 이벤트
	UPROPERTY(BlueprintAssignable, Category = "PD|Weapon|Events")
	FOnWeaponFired OnWeaponFired;

	UPROPERTY(BlueprintAssignable, Category = "PD|Weapon|Events")
	FOnWeaponLevelChanged OnWeaponLevelChanged;

	UPROPERTY(BlueprintAssignable, Category = "PD|Weapon|Events")
	FOnWeaponReloaded OnWeaponReloaded;

public:

	// 무기 액션
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PD|Weapon")
	void Fire();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PD|Weapon")
	void Reload();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PD|Weapon")
	void OnEquip(AActor* NewOwner);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PD|Weapon")
	void OnUnequip();

	// 애님 노티파이
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PD|Weapon|Animation")
	void EjectShell();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PD|Weapon|Animation")
	void DropMagazine();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PD|Weapon|Animation")
	void AttachNewMagazine();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PD|Weapon|Animation")
	void OnBoltPulled();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PD|Weapon|Animation")
	void OnBoltReleased();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PD|Weapon|Animation")
	void OnShellInserted();

	virtual void Interact_Implementation(AActor* Interactor) override;

	// 유틸리티
	UFUNCTION(BlueprintCallable, Category = "PD|Weapon")
	void UpgradeLevel();

	UFUNCTION(BlueprintCallable, Category = "PD|Weapon")
	void SetLevel(int32 NewLevel);

	UFUNCTION(BlueprintCallable, Category = "PD|Weapon")
	void SetDropped(bool bDropped);

	UFUNCTION(BlueprintPure, Category = "PD|Weapon")
	const FWeaponLevelStats& GetCurrentStats() const;

	UFUNCTION(BlueprintPure, Category = "PD|Weapon")
	FORCEINLINE int32 GetCurrentLevel()     const { return CurrentLevel; }

	UFUNCTION(BlueprintPure, Category = "PD|Weapon")
	FORCEINLINE int32 GetCurrentAmmo()      const { return CurrentAmmo; }

	UFUNCTION(BlueprintPure, Category = "PD|Weapon")
	FORCEINLINE bool IsReloading()          const { return bIsReloading; }

	UFUNCTION(BlueprintPure, Category = "PD|Weapon")
	FORCEINLINE EWeaponType GetWeaponType() const { return WeaponType; }

	UFUNCTION(BlueprintPure, Category = "PD|Weapon")
	FORCEINLINE AActor* GetWeaponOwner()    const { return WeaponOwner.Get(); }

	UFUNCTION(BlueprintPure, Category = "PD|Weapon")
	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }

	UFUNCTION(BlueprintPure, Category="PD|Weapon|GAS")
	FORCEINLINE FGameplayTag GetWeaponTypeTag() const { return WeaponTypeTag; }

	UFUNCTION(BlueprintPure, Category="PD|Weapon|Animation")
	FORCEINLINE TSubclassOf<UAnimInstance> GetWeaponAnimLayerClass() const { return WeaponAnimLayerClass; }

	UFUNCTION(BlueprintPure, Category="PD|Weapon|Animation")
	FORCEINLINE FName GetLeftHandGripSocket() const { return LeftHandGripSocket; }
	
	void FinishReload();

protected:
	bool CanFire() const;
	void ApplyDamage(AActor* HitActor, float DamageAmount);
	void PostFire();
	void ResetFireCooldown();

	void PlayWeaponMontage(UAnimMontage* Montage, FName StartSection = NAME_None);
	bool IsPlayingMontage(UAnimMontage* Montage) const;
	void StopWeaponMontage(UAnimMontage* Montage);
	void BindMontageEndedForReload(UAnimMontage* Montage);

	void ApplyRecoil();      // PostFire()에서 호출
	APlayerController* GetOwnerPlayerController() const;

	FTimerHandle SpreadRecoveryHandle;
	FTimerHandle MeshRecoilRecoveryHandle;
	FRotator     OriginalMeshRelRotation;

	UFUNCTION()
	void TickSpreadRecovery();

	UFUNCTION()
	void TickMeshRecoilRecovery();

private:
	UFUNCTION()
	void OnReloadMontageEnded(UAnimMontage* Montage, bool bInterrupted);
};



