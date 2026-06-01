#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "Type/Types.h"
#include "PDQuestData.generated.h"

UENUM(BlueprintType)
enum class EPDQuestState : uint8
{
	Inactive	UMETA(DisplayName = "Inactive"),
	Active		UMETA(DisplayName = "Active"),
	Completed	UMETA(DisplayName = "Completed"),
	Rewarded	UMETA(DisplayName = "Rewarded")
};

UENUM(BlueprintType)
enum class EPDQuestCategory : uint8
{
	Main		UMETA(DisplayName = "Main"),
	Side		UMETA(DisplayName = "Side"),
	Event		UMETA(DisplayName = "Event"),
	Tutorial	UMETA(DisplayName = "Tutorial")
};

UENUM(BlueprintType)
enum class EPDQuestObjectiveType : uint8
{
	ItemAcquired		UMETA(DisplayName = "아이템 획득"),
	QuestItemAcquired	UMETA(DisplayName = "퀘스트 아이템 획득"),
	EnemyKilled			UMETA(DisplayName = "적 처치"),
	QuestEnemyKilled	UMETA(DisplayName = "퀘스트 적 처치"),
	ReachLocation		UMETA(DisplayName = "특정 위치에 도달"),
	TalkToNPC			UMETA(DisplayName = "지정 NPC 말걸기"),
	ItemDropped			UMETA(DisplayName = "아이템 버리기")
};

USTRUCT(BlueprintType)
struct FPDQuestObjective
{
	GENERATED_BODY()

public:
	UPROPERTY(Transient, BlueprintReadOnly, Category = "PD|Quest")
	FName ObjectiveID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Quest")
	EPDQuestObjectiveType ObjectiveType = EPDQuestObjectiveType::ItemAcquired;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Quest")
	FName TargetID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Quest")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Quest")
	int32 RequiredCount = 1;

	FName GetProgressKey() const
	{
		if (!ObjectiveID.IsNone())
		{
			return ObjectiveID;
		}

		const FString KeyString = FString::Printf(TEXT("%d_%s"), static_cast<int32>(ObjectiveType), *TargetID.ToString());
		return FName(*KeyString);
	}
};

USTRUCT(BlueprintType)
struct FPDQuestRewardItem
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Reward")
	FName ItemID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Reward", meta=(ClampMin="1"))
	int32 Quantity = 1;
};

USTRUCT(BlueprintType)
struct FPDQuestReward
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Reward")
	int32 RewardGold = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Reward")
	TArray<FPDQuestRewardItem> RewardItems;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Reward")
	int32 RewardTraderReputationExp = 0;
};

USTRUCT(BlueprintType)
struct FPDQuestData : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(Transient, BlueprintReadOnly, Category = "PD|Quest")
	FName QuestID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Quest")
	FText QuestName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Quest")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Quest")
	EPDQuestCategory Category = EPDQuestCategory::Side;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Quest")
	TObjectPtr<UTexture2D> QuestIcon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Quest")
	TArray<FPDQuestObjective> Objectives;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Reward")
	FPDQuestReward Reward;
};

USTRUCT(BlueprintType)
struct FPDQuestProgress
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Quest")
	FPDQuestData QuestData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Quest")
	TMap<FName, int32> ObjectiveProgress;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Quest")
	EPDQuestState State = EPDQuestState::Inactive;
};
