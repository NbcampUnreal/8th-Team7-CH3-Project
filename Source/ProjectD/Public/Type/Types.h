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
	EPDItemGrade ItemGrade = EPDItemGrade::Grade1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsQuestItem = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
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

USTRUCT(BlueprintType)
struct FPDInventorySlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FPDItemData ItemData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Quantity = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsEmpty = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 ModificationLevel = 0;

	bool IsEmpty() const
	{
		return bIsEmpty || Quantity <= 0 || ItemData.ItemID.IsNone();
	}

	void Clear()
	{
		ItemData = FPDItemData();
		Quantity = 0;
		bIsEmpty = true;
		ModificationLevel = 0;
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

	/**
	 * 레이드 입장 시 들고 가는 아이템 목록.
	 * 허브에서 로드아웃 확정 시 스태시에서 빼서 여기 저장.
	 * StartRaid() 안에서 플레이어 인벤토리로 이전 후 클리어.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FPDInventorySlot> RaidLoadout;

	/** 레이드에 들고 가는 골드. 사망 시 소실, 탈출 성공 시 PlayerData.Gold 에 합산. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 RaidGold = 0;
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
