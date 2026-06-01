#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Type/Types.h"
#include "GameplayTag/PDGameplayTags.h"
#include "Animation/AnimMontage.h"
#include "Interfaces/PDInteractable.h"
#include "PDWeaponBase.generated.h"

class APDWeaponBase;
class USphereComponent;
class USoundBase;
class UGCN_Weapon_Equip;
class UGCN_Weapon_Swing;
class UGCN_Weapon_MeleeHit;
class UTexture2D;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeaponLevelChanged, APDWeaponBase*, Weapon, int32, NewLevel);

UCLASS(Abstract, Blueprintable)
class PROJECTD_API APDWeaponBase : public AActor, public IPDInteractable
{
	GENERATED_BODY()
	friend class APDPlayerCharacter;

public:
	APDWeaponBase();

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon")
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon")
	TObjectPtr<USphereComponent> PickupCollision;


	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	FGameplayTag WeaponTypeTag;


	UPROPERTY(ReplicatedUsing=OnRep_WeaponIdentity, EditAnywhere, BlueprintReadOnly, Category="Weapon")
	FName ItemID;

	UPROPERTY(ReplicatedUsing=OnRep_WeaponIdentity, VisibleAnywhere, BlueprintReadOnly, Category="Weapon")
	FGuid ItemInstanceID;


	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Animation")
	TSubclassOf<UAnimInstance> WeaponAnimLayerClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Animation")
	FName LeftHandGripSocket = TEXT("LeftHandGrip");


	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Stats")
	TArray<FWeaponLevelStats> LevelStats;


	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|FX")
	TObjectPtr<USoundBase> EquipSound;

	/** HUD/UI 실루엣 텍스처. 무기 BP 의 Details 패널에서 무기별로 지정. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|UI")
	TSoftObjectPtr<UTexture2D> UISilhouette;

	UPROPERTY(ReplicatedUsing=OnRep_WeaponIdentity, EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	EWeaponType WeaponType = EWeaponType::None;


	UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category="Weapon|State")
	int32 CurrentLevel = 1;

	UPROPERTY(ReplicatedUsing=OnRep_Dropped, VisibleAnywhere, BlueprintReadOnly, Category="Weapon|State")
	bool bIsDropped = false;

	UPROPERTY(ReplicatedUsing=OnRep_WeaponOwner, VisibleAnywhere, BlueprintReadOnly, Category="Weapon|State")
	TObjectPtr<AActor> WeaponOwner;

	UFUNCTION()
	void OnRep_Dropped();

	UFUNCTION()
	void OnRep_WeaponOwner();

	UFUNCTION()
	void OnRep_WeaponIdentity();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastOnPickedUp();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastOnEquipped(AActor* NewOwner, bool bShouldBeVisible);

	void ApplyReplicatedWeaponOwner();
	void ApplyPickedUpPresentation();
	void ApplyEquippedPresentation(AActor* NewOwner, bool bShouldBeVisible);

public:
	UPROPERTY(BlueprintAssignable, Category="Weapon")
	FOnWeaponLevelChanged OnWeaponLevelChanged;

	UFUNCTION(BlueprintImplementableEvent, Category="Weapon")
	void OnPickupFailed();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Weapon")
	void Fire();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Weapon")
	void Reload();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Weapon")
	void OnEquip(AActor* NewOwner);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Weapon")
	void OnUnequip();

	virtual void Interact_Implementation(AActor* Interactor) override;

	UFUNCTION(BlueprintCallable, Category="Weapon")
	void UpgradeLevel();

	UFUNCTION(BlueprintCallable, Category="Weapon")
	void SetLevel(int32 NewLevel);

	UFUNCTION(BlueprintCallable, Category="Weapon")
	void SetDropped(bool bDropped);

	UFUNCTION(BlueprintPure, Category="Weapon")
	const FWeaponLevelStats& GetCurrentStats() const;

	FORCEINLINE int32              GetCurrentLevel()        const { return CurrentLevel; }
	FORCEINLINE EWeaponType        GetWeaponType()          const { return WeaponType; }
	FORCEINLINE AActor*            GetWeaponOwner()         const { return WeaponOwner.Get(); }
	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh()     const { return WeaponMesh; }
	FORCEINLINE FGameplayTag       GetWeaponTypeTag()       const { return WeaponTypeTag; }
	FORCEINLINE TSubclassOf<UAnimInstance> GetWeaponAnimLayerClass() const { return WeaponAnimLayerClass; }
	FORCEINLINE FName              GetLeftHandGripSocket()  const { return LeftHandGripSocket; }
	FORCEINLINE FName              GetItemID()              const { return ItemID; }
	FORCEINLINE FGuid             GetItemInstanceID()       const { return ItemInstanceID; }

	/** UISilhouette 동기 로드. 위젯이 무기 스왑 시점에 호출. nullptr 가능. */
	UFUNCTION(BlueprintPure, Category="Weapon|UI")
	UTexture2D* GetUISilhouette() const;

protected:
	FVector GetAimDirectionFromOwner(const FVector& StartLocation) const;

	friend class UGCN_Weapon_Equip;
	friend class UGCN_Weapon_Swing;
	friend class UGCN_Weapon_MeleeHit;
};
