#include "Data/PDQuestComponent.h"
#include "Items/PDInventoryComponent.h"

UPDQuestComponent::UPDQuestComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UPDQuestComponent::AddQuest(const FPDQuestData& QuestData)
{
	if (QuestData.QuestID.IsNone() || FindQuest(QuestData.QuestID))
	{
		return false;
	}

	FPDQuestProgress NewProgress;
	NewProgress.QuestData = QuestData;
	NewProgress.State = EPDQuestState::Active;

	for (const FPDQuestObjective& Objective : QuestData.Objectives)
	{
		if (!Objective.ObjectiveID.IsNone())
		{
			NewProgress.ObjectiveProgress.Add(Objective.ObjectiveID, 0);
		}
	}

	ActiveQuests.Add(NewProgress);
	return true;
}

bool UPDQuestComponent::UpdateObjectiveProgress(FName QuestID, FName ObjectiveID, int32 Amount)
{
	FPDQuestProgress* QuestProgress = FindQuest(QuestID);
	if (!QuestProgress || QuestProgress->State != EPDQuestState::Active || ObjectiveID.IsNone() || Amount <= 0)
	{
		return false;
	}

	bool bKnownObjective = false;
	int32 RequiredCount = 0;

	for (const FPDQuestObjective& Objective : QuestProgress->QuestData.Objectives)
	{
		if (Objective.ObjectiveID == ObjectiveID)
		{
			bKnownObjective = true;
			RequiredCount = FMath::Max(1, Objective.RequiredCount);
			break;
		}
	}

	if (!bKnownObjective)
	{
		return false;
	}

	int32& Progress = QuestProgress->ObjectiveProgress.FindOrAdd(ObjectiveID);
	Progress = FMath::Clamp(Progress + Amount, 0, RequiredCount);

	RefreshQuestState(*QuestProgress);
	return true;
}

bool UPDQuestComponent::IsQuestCompleted(FName QuestID) const
{
	const FPDQuestProgress* QuestProgress = FindQuest(QuestID);
	return QuestProgress && (QuestProgress->State == EPDQuestState::Completed || QuestProgress->State == EPDQuestState::Rewarded);
}

bool UPDQuestComponent::GiveReward(FName QuestID, UPDInventoryComponent* InventoryComponent)
{
	FPDQuestProgress* QuestProgress = FindQuest(QuestID);
	if (!QuestProgress || QuestProgress->State != EPDQuestState::Completed || !InventoryComponent)
	{
		return false;
	}

	for (const FPDItemData& RewardItem : QuestProgress->QuestData.Reward.RewardItems)
	{
		InventoryComponent->AddItemPartial(RewardItem, 1);
	}

	InventoryComponent->AddGold(QuestProgress->QuestData.Reward.RewardGold);
	QuestProgress->State = EPDQuestState::Rewarded;
	return true;
}

FPDQuestProgress* UPDQuestComponent::FindQuest(FName QuestID)
{
	for (FPDQuestProgress& QuestProgress : ActiveQuests)
	{
		if (QuestProgress.QuestData.QuestID == QuestID)
		{
			return &QuestProgress;
		}
	}

	return nullptr;
}

const FPDQuestProgress* UPDQuestComponent::FindQuest(FName QuestID) const
{
	for (const FPDQuestProgress& QuestProgress : ActiveQuests)
	{
		if (QuestProgress.QuestData.QuestID == QuestID)
		{
			return &QuestProgress;
		}
	}

	return nullptr;
}

void UPDQuestComponent::RefreshQuestState(FPDQuestProgress& QuestProgress)
{
	if (QuestProgress.QuestData.Objectives.IsEmpty())
	{
		return;
	}

	for (const FPDQuestObjective& Objective : QuestProgress.QuestData.Objectives)
	{
		if (Objective.ObjectiveID.IsNone())
		{
			return;
		}

		const int32 CurrentProgress = QuestProgress.ObjectiveProgress.FindRef(Objective.ObjectiveID);
		if (CurrentProgress < FMath::Max(1, Objective.RequiredCount))
		{
			return;
		}
	}

	QuestProgress.State = EPDQuestState::Completed;
}
