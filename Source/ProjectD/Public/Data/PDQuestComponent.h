#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/PDQuestData.h"
#include "Engine/DataTable.h"
#include "PDQuestComponent.generated.h"

class FLifetimeProperty;
class UPDInventoryComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPDOnQuestUpdated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPDOnQuestStateChanged, FName, QuestID, EPDQuestState, NewState);

USTRUCT()
struct FPDReplicatedQuestObjectiveProgress
{
	GENERATED_BODY()

	UPROPERTY()
	FName ProgressKey = NAME_None;

	UPROPERTY()
	int32 Amount = 0;
};

USTRUCT()
struct FPDReplicatedQuestProgress
{
	GENERATED_BODY()

	UPROPERTY()
	FPDQuestData QuestData;

	UPROPERTY()
	TArray<FPDReplicatedQuestObjectiveProgress> ObjectiveProgress;

	UPROPERTY()
	EPDQuestState State = EPDQuestState::Inactive;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTD_API UPDQuestComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPDQuestComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|Quest")
	TObjectPtr<UDataTable> QuestTable = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|Quest")
	TArray<FPDQuestProgress> ActiveQuests;

	UPROPERTY(ReplicatedUsing=OnRep_TrackedQuestID, EditAnywhere, BlueprintReadWrite, Category="PD|Quest")
	FName TrackedQuestID;

	UPROPERTY(BlueprintAssignable, Category="PD|Quest")
	FPDOnQuestUpdated OnQuestUpdated;

	UPROPERTY(BlueprintAssignable, Category="PD|Quest")
	FPDOnQuestStateChanged OnQuestStateChanged;

	UFUNCTION(BlueprintCallable, Category="PD|Quest")
	bool AddQuest(const FPDQuestData& QuestData);

	UFUNCTION(BlueprintCallable, Category="PD|Quest")
	bool AddQuestByID(FName QuestID);

	UFUNCTION(BlueprintCallable, Category="PD|Quest")
	bool UpdateObjectiveProgress(FName QuestID, FName ObjectiveID, int32 Amount = 1);

	UFUNCTION(BlueprintCallable, Category="PD|Quest")
	bool ReportQuestObjectiveEvent(EPDQuestObjectiveType ObjectiveType, FName TargetID = NAME_None, int32 Amount = 1);

	UFUNCTION(BlueprintCallable, Category="PD|Quest")
	bool ReportItemAcquired(FName ItemID, int32 Amount = 1);

	UFUNCTION(BlueprintCallable, Category="PD|Quest")
	bool ReportQuestItemAcquired(FName ItemID, int32 Amount = 1);

	UFUNCTION(BlueprintCallable, Category="PD|Quest")
	bool ReportEnemyKilled(FName EnemyID, int32 Amount = 1);

	UFUNCTION(BlueprintCallable, Category="PD|Quest")
	bool ReportQuestEnemyKilled(FName EnemyID, int32 Amount = 1);

	UFUNCTION(BlueprintCallable, Category="PD|Quest")
	bool ReportLocationReached(FName LocationID, int32 Amount = 1);

	UFUNCTION(BlueprintCallable, Category="PD|Quest")
	bool ReportNPCTalked(FName NPCID, int32 Amount = 1);

	UFUNCTION(BlueprintCallable, Category="PD|Quest")
	bool ReportItemDropped(FName ItemID, int32 Amount = 1);

	UFUNCTION(BlueprintCallable, Category="PD|Quest")
	bool IsQuestCompleted(FName QuestID) const;

	UFUNCTION(BlueprintCallable, Category="PD|Quest")
	bool GiveReward(FName QuestID, UPDInventoryComponent* InventoryComponent);

	UFUNCTION(BlueprintCallable, Category="PD|Quest")
	bool SetTrackedQuest(FName QuestID);

	UFUNCTION(BlueprintCallable, Category="PD|Quest")
	void ClearTrackedQuest();

	UFUNCTION(BlueprintPure, Category="PD|Quest")
	bool GetQuestProgress(FName QuestID, FPDQuestProgress& OutQuestProgress) const;

	UFUNCTION(BlueprintPure, Category="PD|Quest")
	TArray<FPDQuestProgress> GetQuestsByState(EPDQuestState State) const;

	UFUNCTION(BlueprintPure, Category="PD|Quest")
	TArray<FPDQuestData> GetAvailableQuests() const;

	UFUNCTION(BlueprintPure, Category="PD|Quest")
	int32 GetObjectiveProgress(FName QuestID, FName ObjectiveID) const;

	UFUNCTION(BlueprintPure, Category="PD|Quest")
	int32 GetObjectiveRequiredCount(FName QuestID, FName ObjectiveID) const;

	UFUNCTION(BlueprintPure, Category="PD|Quest")
	float GetQuestProgressRatio(FName QuestID) const;

private:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_ReplicatedActiveQuests();

	UFUNCTION()
	void OnRep_TrackedQuestID();

	UPROPERTY(ReplicatedUsing=OnRep_ReplicatedActiveQuests)
	TArray<FPDReplicatedQuestProgress> ReplicatedActiveQuests;

	void SyncActiveQuestsToReplication();

	FPDQuestProgress* FindQuest(FName QuestID);
	const FPDQuestProgress* FindQuest(FName QuestID) const;
	const FPDQuestData* FindQuestData(FName QuestID) const;
	bool DoesObjectiveMatchEvent(const FPDQuestObjective& Objective, EPDQuestObjectiveType ObjectiveType, FName TargetID) const;
	bool ApplyObjectiveProgress(FPDQuestProgress& QuestProgress, const FPDQuestObjective& Objective, int32 Amount);
	bool IsItemCountObjective(const FPDQuestObjective& Objective) const;
	int32 GetInventoryAndStashItemCount(FName ItemID, const UPDInventoryComponent* InventoryComponent = nullptr) const;
	bool RefreshItemCountObjective(FPDQuestProgress& QuestProgress, const FPDQuestObjective& Objective);
	void RefreshAllItemCountObjectives(FPDQuestProgress& QuestProgress);
	int32 RemoveQuestItemsFromStash(FName ItemID, int32 Quantity);
	/** Source legacy: simulate whether quest objective removal + reward grant fits into the player's inventory/equipment. */
	bool CanReceiveQuestReward(const FPDQuestProgress& QuestProgress, const UPDInventoryComponent* InventoryComponent) const;
	bool RemoveQuestObjectiveItems(FPDQuestProgress& QuestProgress, UPDInventoryComponent* InventoryComponent);
	void RefreshQuestState(FPDQuestProgress& QuestProgress);
	void BroadcastQuestUpdated(FName QuestID, EPDQuestState PreviousState, EPDQuestState NewState);
};
