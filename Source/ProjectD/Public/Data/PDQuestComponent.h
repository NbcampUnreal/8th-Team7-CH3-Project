#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/PDQuestData.h"
#include "PDQuestComponent.generated.h"

class UPDInventoryComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTD_API UPDQuestComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPDQuestComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|Quest")
	TArray<FPDQuestProgress> ActiveQuests;

	UFUNCTION(BlueprintCallable, Category="PD|Quest")
	bool AddQuest(const FPDQuestData& QuestData);

	UFUNCTION(BlueprintCallable, Category="PD|Quest")
	bool UpdateObjectiveProgress(FName QuestID, FName ObjectiveID, int32 Amount = 1);

	UFUNCTION(BlueprintCallable, Category="PD|Quest")
	bool IsQuestCompleted(FName QuestID) const;

	UFUNCTION(BlueprintCallable, Category="PD|Quest")
	bool GiveReward(FName QuestID, UPDInventoryComponent* InventoryComponent);

private:
	FPDQuestProgress* FindQuest(FName QuestID);
	const FPDQuestProgress* FindQuest(FName QuestID) const;
	void RefreshQuestState(FPDQuestProgress& QuestProgress);
};
