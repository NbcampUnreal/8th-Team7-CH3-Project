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

	// ── 컴포넌트 ──────────────────────────────────────────────────────────────
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon")
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon")
	TObjectPtr<USphereComponent> PickupCollision;

	// ── 설정 ──────────────────────────────────────────────────────────────────
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	FGameplayTag WeaponTypeTag;

	/** 인벤토리 아이템 ID. DT_Items 행 이름과 일치해야 함. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon")
	FName ItemID;

	/** 무기 장착 시 링크할 애님 레이어 클래스. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Animation")
	TSubclassOf<UAnimInstance> WeaponAnimLayerClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Animation")
	FName LeftHandGripSocket = TEXT("LeftHandGrip");

	/** 레벨별 스탯 배열. [0]=Lv1, [1]=Lv2 … */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Stats")
	TArray<FWeaponLevelStats> LevelStats;

	/** 무기 꺼낼 때 재생 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|FX")
	TObjectPtr<USoundBase> EquipSound;

	// C++ 서브클래스 생성자에서 설정. 에디터 미노출 (태그로 대체 예정)
	EWeaponType WeaponType = EWeaponType::None;

	// ── 런타임 상태 (읽기 전용) ───────────────────────────────────────────────
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon|State")
	int32 CurrentLevel = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon|State")
	bool bIsDropped = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon|State")
	TWeakObjectPtr<AActor> WeaponOwner;

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
	FORCEINLINE EWeaponType        GetWeaponType()          const { return WeaponType; }  // C++ 내부용, 태그로 대체 예정
	FORCEINLINE AActor*            GetWeaponOwner()         const { return WeaponOwner.Get(); }
	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh()     const { return WeaponMesh; }
	FORCEINLINE FGameplayTag       GetWeaponTypeTag()       const { return WeaponTypeTag; }
	FORCEINLINE TSubclassOf<UAnimInstance> GetWeaponAnimLayerClass() const { return WeaponAnimLayerClass; }
	FORCEINLINE FName              GetLeftHandGripSocket()  const { return LeftHandGripSocket; }
	FORCEINLINE FName              GetItemID()              const { return ItemID; }

protected:
	FVector GetAimDirectionFromOwner(const FVector& StartLocation) const;
};
