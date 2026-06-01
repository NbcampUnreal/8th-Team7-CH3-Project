#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/PDQuestData.h"
#include "Widgets/Screen/PDTabbedContent.h"
#include "PDQuestWindowWidget.generated.h"

class USoundBase;
class UButton;
class UScrollBox;
class UTextBlock;
class UVerticalBox;
class UPDInventoryComponent;
class UPDQuestComponent;
class UPDQuestListItemWidget;

UCLASS(Abstract, BlueprintType, meta=(DisableNativeTick))
class PROJECTD_API UPDQuestWindowWidget : public UUserWidget, public IPDTabbedContent
{
	GENERATED_BODY()

public:
	// IPDTabbedContent
	virtual void InitializeForOwner(APlayerController* OwnerPC) override;
	virtual void OnTabShown() override;
	virtual void OnTabHidden() override;

	UFUNCTION(BlueprintCallable, Category="PD|Quest")
	void InitializeQuestWindow(UPDQuestComponent* InQuestComponent, UPDInventoryComponent* InInventoryComponent);

	UFUNCTION(BlueprintCallable, Category="PD|Quest")
	void RefreshQuestList();

	UFUNCTION(BlueprintCallable, Category="PD|Quest")
	void RefreshQuestDetail();

	UFUNCTION(BlueprintCallable, Category="PD|Quest")
	void SelectQuest(FName QuestID);

	UFUNCTION(BlueprintCallable, Category="PD|Quest")
	void SetTabState(EPDQuestState NewState);

	UFUNCTION(BlueprintPure, Category="PD|Quest")
	bool IsQuestSelected(FName QuestID) const { return SelectedQuestID == QuestID; }

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|UI Sound")
	TObjectPtr<USoundBase> ButtonClickSound;

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	TObjectPtr<UButton> BTN_Available;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	TObjectPtr<UButton> BTN_Active;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	TObjectPtr<UButton> BTN_Completed;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	TObjectPtr<UScrollBox> SB_QuestList;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	TObjectPtr<UTextBlock> TXT_QuestTitle;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	TObjectPtr<UTextBlock> TXT_QuestDescription;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	TObjectPtr<UVerticalBox> VB_Objectives;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	TObjectPtr<UVerticalBox> VB_Rewards;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	TObjectPtr<UButton> BTN_Action;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	TObjectPtr<UTextBlock> TXT_ActionButton;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Quest")
	TSubclassOf<UPDQuestListItemWidget> QuestListItemClass;

private:
	UFUNCTION()
	void HandleQuestUpdated();

	UFUNCTION()
	void HandleAvailableClicked();

	UFUNCTION()
	void HandleActiveClicked();

	UFUNCTION()
	void HandleCompletedClicked();

	UFUNCTION()
	void HandleActionClicked();

	void BindQuestComponent();
	void UnbindQuestComponent();
	void BindButtons();
	void ClearDetail();
	bool GetSelectedQuestData(FPDQuestData& OutQuestData, FPDQuestProgress& OutQuestProgress, bool& bOutHasProgress) const;
	UPDQuestComponent* FindQuestComponent() const;
	UPDInventoryComponent* FindInventoryComponent() const;
	void AddDetailLine(UVerticalBox* TargetBox, const FText& Text) const;
	void UpdateTabButtonStyle() const;
	void ApplyTabButtonStyle(UButton* TargetButton, bool bSelected) const;
	FText GetActionButtonText(EPDQuestState State) const;
	FText MakeProgressText(const FPDQuestData& QuestData, const FPDQuestProgress* QuestProgress) const;
	void GetObjectiveTotalCounts(const FPDQuestData& QuestData, const FPDQuestProgress* QuestProgress, int32& OutCurrent, int32& OutRequired) const;

	UPROPERTY(Transient)
	TObjectPtr<UPDQuestComponent> QuestComponent;

	UPROPERTY(Transient)
	TObjectPtr<UPDInventoryComponent> InventoryComponent;

	EPDQuestState CurrentTabState = EPDQuestState::Inactive;
	FName SelectedQuestID;
};
