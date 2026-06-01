#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "Templates/SubclassOf.h"
#include "Types.generated.h"

class APDWeaponBase;
class UGameplayEffect;
class AActor;
class UDamageType;

UENUM(BlueprintType)
enum class EPDItemType : uint8
{
	Equipment  UMETA(DisplayName = "Equipment"),
	Consumable UMETA(DisplayName = "Consumable"),
	Misc       UMETA(DisplayName = "Misc"),
};

UENUM(BlueprintType)
enum class EPDItemGrade : uint8
{
	Grade1 UMETA(DisplayName = "Grade 1"),
	Grade2 UMETA(DisplayName = "Grade 2"),
	Grade3 UMETA(DisplayName = "Grade 3"),
	Grade4 UMETA(DisplayName = "Grade 4"),
	Grade5 UMETA(DisplayName = "Grade 5"),
};

UENUM(BlueprintType)
enum class EPDItemFilterTab : uint8
{
	Equipment  UMETA(DisplayName = "Equipment"),
	Consumable UMETA(DisplayName = "Consumable"),
	Misc       UMETA(DisplayName = "Misc"),
};



UENUM(BlueprintType)
enum class EPDItemSortMode : uint8
{
	None UMETA(DisplayName = "None"),
	Name UMETA(DisplayName = "Name"),
	Type UMETA(DisplayName = "Type"),
};

UENUM(BlueprintType)
enum class EPDEquipmentSlotType : uint8
{
	None   UMETA(DisplayName = "None"),
	Weapon UMETA(DisplayName = "Weapon"),
	Head   UMETA(DisplayName = "Head"),
	Armor  UMETA(DisplayName = "Armor"),
	Bag    UMETA(DisplayName = "Bag"),
};

UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	None    UMETA(DisplayName = "None"),
	Rifle   UMETA(DisplayName = "Rifle"),
	Shotgun UMETA(DisplayName = "Shotgun"),
	Sniper  UMETA(DisplayName = "Sniper"),
	Melee   UMETA(DisplayName = "Melee"),
};

UENUM(BlueprintType)
enum class EPDItemType : uint8
{
	Equipment  UMETA(DisplayName = "Equipment"),
	Consumable UMETA(DisplayName = "Consumable"),
	Misc       UMETA(DisplayName = "Misc"),
};

USTRUCT(BlueprintType)
struct FPDItemData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ItemID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EPDItemType ItemType = EPDItemType::Misc;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
<<<<<<< HEAD
	EPDItemGrade ItemGrade = EPDItemGrade::Grade1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsQuestItem = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
=======
>>>>>>> origin/main
	TObjectPtr<UTexture2D> Icon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Price = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Weight = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float BagCapacityWeight = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaxStack = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EPDEquipmentSlotType EquipmentSlotType = EPDEquipmentSlotType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<APDWeaponBase> WeaponClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EWeaponType WeaponType = EWeaponType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UGameplayEffect> UseEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0", EditCondition="ItemType == EPDItemType::Consumable", EditConditionHides))
	float UseDuration = 0.f;
};

/**
 * 무기 인스턴스의 휘발성 런타임 상태 — 인벤토리/장비/퀵슬롯 사이를 이동할 때 보존되어야 함.
 * 액터(`APDRangedWeaponBase`)는 스왑 시 destroy/spawn 되므로 이 구조체가 영속 매개체.
 * CurrentAmmo = -1 은 "아직 저장된 값 없음" 센티넬 — spawn 측이 기본(풀충탄) 동작 유지.
 */
USTRUCT(BlueprintType)
struct FPDWeaponInstanceState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CurrentAmmo = -1;

	bool HasPersistedAmmo() const { return CurrentAmmo >= 0; }

	void Reset()
	{
		CurrentAmmo = -1;
	}
};

USTRUCT(BlueprintType)
struct FPDInventorySlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGuid ItemInstanceID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FPDItemData ItemData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Quantity = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsEmpty = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 ModificationLevel = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FPDWeaponInstanceState WeaponState;

	bool IsEmpty() const
	{
		return bIsEmpty || Quantity <= 0 || ItemData.ItemID.IsNone();
	}

	void AssignNewInstanceID()
	{
		ItemInstanceID = FGuid::NewGuid();
	}

	void EnsureInstanceID()
	{
		if (!IsEmpty() && !ItemInstanceID.IsValid())
		{
			AssignNewInstanceID();
		}
	}

	void Clear()
	{
		ItemInstanceID = FGuid();
		ItemData = FPDItemData();
		Quantity = 0;
		bIsEmpty = true;
		ModificationLevel = 0;
		WeaponState.Reset();
	}
};

USTRUCT(BlueprintType)
struct FPDModificationMaterialRequirement
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName RequiredItemID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Quantity = 0;
};

UENUM(BlueprintType)
enum class EPDModificationBoostType : uint8
{
	None UMETA(DisplayName = "None"),
	Low UMETA(DisplayName = "Low"),
	Mid UMETA(DisplayName = "Mid"),
	High UMETA(DisplayName = "High"),
};

USTRUCT(BlueprintType)
struct FPDModificationRecipeData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 TargetLevel = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 GoldCost = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName RequiredItemID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Quantity = 1;

};

USTRUCT(BlueprintType)
struct FPDModificationBoostData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EPDModificationBoostType BoostType = EPDModificationBoostType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName BoostItemID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Quantity = 1;
};

USTRUCT(BlueprintType)
struct FPDModificationPreview
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CurrentModificationLevel = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 TargetModificationLevel = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 TargetGasLevel = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 GoldCost = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SuccessRate = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float BaseSuccessRate = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float BoostSuccessRate = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EPDModificationBoostType SelectedBoostType = EPDModificationBoostType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName BoostItemID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 BoostItemQuantity = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AttackBonus = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DefenseBonus = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FPDModificationMaterialRequirement> RequiredMaterials;
};


USTRUCT(BlueprintType)
struct FPDEquippedItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EPDEquipmentSlotType SlotType = EPDEquipmentSlotType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FPDInventorySlot ItemSlot;

	bool IsEmpty() const
	{
		return SlotType == EPDEquipmentSlotType::None || ItemSlot.IsEmpty();
	}

	void Clear()
	{
		SlotType = EPDEquipmentSlotType::None;
		ItemSlot.Clear();
	}
};

USTRUCT(BlueprintType)
struct FPDPlayerData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Gold = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Experience = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FPDInventorySlot> StashItems;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 StashUpgradeLevel = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 TraderReputationExp = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 TraderReputationLevel = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FPDInventorySlot> RaidLoadout;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 RaidGold = 0;

	// per-player 영속 장착 장비. 탈출 시 캡처 → 진입 시 재장착 (공용 스태시와 별개).
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FPDEquippedItem> EquippedItems;

	// per-player 영속 퀵슬롯 아이템(보조무기/소모품). 인덱스 = 퀵슬롯 슬롯 번호, 빈 슬롯 = 미할당.
	// 탈출 시 캡처(스태시 제외) → 진입 시 인벤토리+퀵슬롯 복원.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FPDInventorySlot> QuickSlotKeptItems;
};

USTRUCT(BlueprintType)
struct FPDDamageInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float BaseDamage = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TWeakObjectPtr<AActor> Instigator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UDamageType> DamageTypeClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FHitResult HitResult;
};


UENUM(BlueprintType)
enum class EBodyPart : uint8
{
	None,
	Head,
	Torso,
	Arm_L,
	Arm_R,
	Leg_L,
	Leg_R
};

USTRUCT(BlueprintType)
struct FBodyPartMapping
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName HitBoxName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EBodyPart BodyPart = EBodyPart::None;
};

UENUM(BlueprintType)
enum class ERaidState : uint8
{
	Idle,
	InProgress,
	Extracting,
	Ended
};

// 존 점유 기반 트래블 카운트다운의 종류. None = 비활성.
UENUM(BlueprintType)
enum class EPDZoneTravelType : uint8
{
	None       UMETA(DisplayName = "None"),
	RaidEntry  UMETA(DisplayName = "Raid Entry"),   // Base → Raid
	Extraction UMETA(DisplayName = "Extraction")    // Raid → Base
};


UENUM(BlueprintType)
enum class EWeaponSlot : uint8
{
	Slot1_Rifle   UMETA(DisplayName = "Rifle"),
	Slot2_Shotgun UMETA(DisplayName = "Shotgun"),
	Slot3_Sniper  UMETA(DisplayName = "Sniper"),
	Slot4_Melee   UMETA(DisplayName = "Melee"),
	None          UMETA(DisplayName = "None"),
};

UENUM(BlueprintType)
enum class EFireMode : uint8
{
	Auto   UMETA(DisplayName = "Auto"),
	Single UMETA(DisplayName = "Single"),
};

USTRUCT(BlueprintType)
struct FWeaponLevelStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float Damage = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float FireRate = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float Range = 1500.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	int32 MaxAmmo = 30;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float ReloadTime = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float Accuracy = 0.95f;
};


UENUM(BlueprintType)
enum class EUILayer : uint8
{
	Frontend UMETA(DisplayName = "Frontend"), 
	GameMenu UMETA(DisplayName = "Game Menu"),
	Modal    UMETA(DisplayName = "Modal"),
};
