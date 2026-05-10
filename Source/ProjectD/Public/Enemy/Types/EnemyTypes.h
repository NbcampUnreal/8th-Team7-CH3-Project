#pragma once

#include "CoreMinimal.h"
#include "EnemyTypes.generated.h"

class APDItemBase;

/** 적 FSM 상태. uint8 기반이므로 BlackboardKey/네트워크 비용 최소. */
UENUM(BlueprintType)
enum class EPDEnemyState : uint8
{
	Idle    UMETA(DisplayName = "Idle"),
	Alert   UMETA(DisplayName = "Alert"),
	Chase   UMETA(DisplayName = "Chase"),
	Combat  UMETA(DisplayName = "Combat"),
	Dead    UMETA(DisplayName = "Dead"),
};

/** Stamina(스태미너) 등급. Biped만 사용, 그 외는 None. */
UENUM(BlueprintType)
enum class EPDStaminaStatus : uint8
{
	None       UMETA(DisplayName = "None"),
	Full       UMETA(DisplayName = "Full"),
	Optimal    UMETA(DisplayName = "Optimal"),
	Low        UMETA(DisplayName = "Low"),
	Exhausted  UMETA(DisplayName = "Exhausted"),
};

/**
 * 사망 시 드랍할 아이템 한 줄.
 * 디자이너가 EnemyBase BP의 LootTable에 항목을 추가해 데이터 주도로 정의.
 */
USTRUCT(BlueprintType)
struct FPDLootEntry
{
	GENERATED_BODY()

	/** 스폰할 아이템 액터 클래스. APDItemBase 또는 그 자식. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Loot")
	TSubclassOf<APDItemBase> ItemClass;

	/** 0~1. 0 이면 항상 미드랍, 1 이면 항상 드랍. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Loot", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DropChance = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Loot", meta = (ClampMin = "1"))
	int32 MinQuantity = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Loot", meta = (ClampMin = "1"))
	int32 MaxQuantity = 1;
};
