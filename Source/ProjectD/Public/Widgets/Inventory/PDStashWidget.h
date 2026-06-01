#pragma once

#include "CoreMinimal.h"
#include "Widgets/PDActivatableBase.h"
#include "Widgets/Inventory/PDInventoryDragDropOperation.h"
<<<<<<< HEAD
#include "Items/Containers/PDStashComponent.h"
=======
>>>>>>> origin/main
#include "PDStashWidget.generated.h"

class USoundBase;
class UUniformGridPanel;
class UPDStashComponent;
class UPDInventoryComponent;
class UPDQuickSlotComponent;
class UPDInventorySlotWidget;
class UUserWidget;
class UPDQuantityPopupWidget;
class UButton;
class UWidget;
class USizeBox;
class UTextBlock;

UCLASS(BlueprintType, Blueprintable)
class PROJECTD_API UPDStashWidget : public UPDActivatableBase
{
	GENERATED_BODY()

public:
	// PCÍ∞Ä Î∞ïÏä§ ??????Î≤??∏Ï∂ú. ?¥Îñ§ Î∞ïÏä§???∞Ïù¥?∞Î? Î≥¥Ïó¨Ï§ÑÏ? ÏßÄ??
	UFUNCTION(BlueprintCallable, Category = "PD|Stash")
	void InitializeStash(UPDStashComponent* InStashComponent);

	UFUNCTION(BlueprintCallable, Category = "PD|Stash")
	void RefreshStashGrid();

	UFUNCTION(BlueprintCallable, Category = "PD|Stash")
	void SetStashFilterTab(EPDItemFilterTab NewFilterTab);

	UFUNCTION(BlueprintCallable, Category = "PD|Stash")
	void SetStashSortMode(EPDItemSortMode NewSortMode);

	UFUNCTION(BlueprintCallable, Category = "PD|Stash")
	EPDStashUpgradeResult RequestStashUpgrade();

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|UI Sound")
	TObjectPtr<USoundBase> ButtonClickSound;

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Stash")
	TSubclassOf<UUserWidget> StashSlotWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Stash")
	int32 FallbackGridColumns = 5;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Stash")
	int32 FallbackGridRows = 4;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Stash")
	FName StashGridWidgetName = TEXT("UniformGridPanel_StashGrid");

	// ScrollBox ?àÏóê?úÎèÑ Stash ?¨Î°Ø??Î∂ÄÎ™??àÏù¥?ÑÏõÉ???òÌï¥ ?∏Î°ú/Í∞ÄÎ°úÎ°ú ?òÏñ¥?òÏ? ?äÎèÑÎ°?Í≥†Ï†ï ?¨Í∏∞Î°?Í∞êÏãº??
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Stash")
	float StashSlotWidth = 120.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Stash")
	float StashSlotHeight = 120.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Stash")
	TSubclassOf<UPDQuantityPopupWidget> QuantityPopupWidgetClass;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "PD|Stash")
	TObjectPtr<UUniformGridPanel> StashGridPanel;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "PD|Stash|Tabs")
	TObjectPtr<UButton> Button_Equipment;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "PD|Stash|Tabs")
	TObjectPtr<UButton> Button_Consumable;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "PD|Stash|Tabs")
	TObjectPtr<UButton> Button_Misc;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "PD|Stash|Sort")
	TObjectPtr<UButton> Button_Sort;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "PD|Stash|Sort")
	TObjectPtr<UButton> Button_SortByName;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "PD|Stash|Sort")
	TObjectPtr<UButton> Button_SortByType;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "PD|Stash|Sort")
	TObjectPtr<UButton> Button_SortTab_Name;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "PD|Stash|Sort")
	TObjectPtr<UButton> Button_SortTab_Type;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "PD|Stash")
	TObjectPtr<UButton> Button_UpgradeStash;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "PD|Stash|Sort")
	TObjectPtr<UWidget> Panel_SortOptions;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "PD|Stash|Sort")
	TObjectPtr<UWidget> Panel_SortTabs;

	UFUNCTION()
	void HandleStashSlotLeftClicked(UPDInventorySlotWidget* SlotWidget, int32 ClickedSlotIndex);

	UFUNCTION()
	void HandleStashSlotItemDropped(UPDInventorySlotWidget* SlotWidget, int32 TargetSlotIndex, UPDInventoryDragDropOperation* DragOperation);

	UFUNCTION()
	void HandleQuantityConfirmed(int32 Quantity);

	UFUNCTION()
	void HandleQuantityCancelled();

<<<<<<< HEAD
	UFUNCTION()
	void HandleEquipmentTabClicked();

	UFUNCTION()
	void HandleConsumableTabClicked();

	UFUNCTION()
	void HandleMiscTabClicked();

	UFUNCTION()
	void HandleSortButtonClicked();

	UFUNCTION()
	void HandleSortByNameClicked();

	UFUNCTION()
	void HandleSortByTypeClicked();

	UFUNCTION()
	void HandleUpgradeStashClicked();

	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Stash")
	void BP_OnStashUpgradeSucceeded(int32 NewUpgradeLevel, int32 NewGridRows);

	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Stash")
	void BP_OnStashUpgradeFailed(EPDStashUpgradeResult Result);

=======
>>>>>>> origin/main
private:
	void ResolveStashGridPanel();
	void BindTabButtons();
	void BindSortButtons();
	void UpdateTabButtonStyle();
	int32 CountOccupiedStashSlotsByType(EPDItemType ItemType) const;
	int32 GetStashDisplaySlotCount() const;
	void CacheFilterTabBaseLabels();
	void CacheFilterTabBaseLabel(EPDItemFilterTab FilterTab, UButton* TargetButton);
	FText GetFilterTabBaseLabel(EPDItemFilterTab FilterTab) const;
	UTextBlock* GetTabButtonTextBlock(UButton* TargetButton) const;
	void SetTabButtonLabel(UButton* TargetButton, const FText& BaseLabel, int32 UsedSlots, int32 MaxSlots) const;
	bool DoesSlotMatchCurrentFilter(const FPDInventorySlot& StashSlotData) const;
	bool DoesItemTypeMatchCurrentFilter(EPDItemType ItemType) const;
	bool CanAcceptDropForCurrentFilter(const UPDInventoryDragDropOperation* DragOperation) const;
	void SortDisplaySlotIndices(TArray<int32>& DisplaySlotIndices, const UPDStashComponent* StashComponent) const;
	void SetSortOptionsVisible(bool bVisible);
	void ToggleSortOptions();
	UPDStashComponent* FindStashComponent() const;
	UPDInventoryComponent* FindInventoryComponent() const;
	UPDQuickSlotComponent* FindQuickSlotComponent() const;
	const FPDInventorySlot* FindStashSlot(int32 SlotIndex) const;
	const FPDInventorySlot* FindSourceSlot(EPDItemContainerType SourceContainerType, int32 SlotIndex) const;
	void TakeStashSlotQuantity(int32 SlotIndex, int32 Quantity);
	void ExecuteStashSlotTransfer(EPDItemContainerType SourceContainerType, int32 SourceSlotIndex, int32 TargetSlotIndex, int32 Quantity);
	void OpenQuantityPopup(int32 SlotIndex, int32 MaxQuantity, const FText& Title);
	void OpenTransferQuantityPopup(EPDItemContainerType SourceContainerType, int32 SourceSlotIndex, int32 TargetSlotIndex, int32 MaxQuantity, const FText& Title);
	bool ShouldOpenTransferQuantityPopup(EPDItemContainerType SourceContainerType, int32 SourceSlotIndex, int32 TargetSlotIndex, int32& OutMaxQuantity, FText& OutTitle) const;
	void ClearQuantityRequest();
	void BindStashChanged();
	void UnbindStashChanged();

	// InitializeStashÎ°?Ï£ºÏûÖÎ∞õÏ? Î∞ïÏä§ Ïª¥Ìè¨?åÌä∏. ?ÑÏ†Ø???¥ÏïÑ?àÎäî ?ôÏïà???Ä??Î∞ïÏä§.
	UPROPERTY(Transient)
	TObjectPtr<UPDStashComponent> TargetStashComponent;

	UPROPERTY(Transient)
	TObjectPtr<UPDStashComponent> BoundStashComponent;

	UPROPERTY(Transient)
	TObjectPtr<UPDQuantityPopupWidget> ActiveQuantityPopup;

	bool bPendingTransferQuantityRequest = false;
	EPDItemContainerType PendingTransferSourceContainerType = EPDItemContainerType::None;
	int32 PendingTransferSourceSlotIndex = INDEX_NONE;
	int32 PendingTransferTargetSlotIndex = INDEX_NONE;

<<<<<<< HEAD
	EPDItemFilterTab CurrentFilterTab = EPDItemFilterTab::Equipment;

	TMap<EPDItemFilterTab, FText> FilterTabBaseLabels;
	EPDItemSortMode CurrentSortMode = EPDItemSortMode::None;

=======
>>>>>>> origin/main
	int32 PendingSlotIndex = INDEX_NONE;
};
