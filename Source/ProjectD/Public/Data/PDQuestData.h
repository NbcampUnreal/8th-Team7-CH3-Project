#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
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

USTRUCT(BlueprintType)
struct FPDQuestObjective
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Quest")
	FName ObjectiveID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Quest")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Quest")
	int32 RequiredCount = 1;
};

USTRUCT(BlueprintType)
struct FPDQuestReward
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Reward")
	int32 RewardGold = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Reward")
	TArray<FPDItemData> RewardItems;
};

USTRUCT(BlueprintType)
struct FPDQuestData : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Quest")
	FName QuestID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Quest")
	FText QuestName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Quest")
	FText Description;

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
