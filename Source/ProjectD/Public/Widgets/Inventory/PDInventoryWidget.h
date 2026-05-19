
#pragma once

#include "CoreMinimal.h"
#include "Widgets/PDActivatableBase.h"
#include "Widgets/Inventory/PDInventoryDragDropOperation.h"
#include "PDInventoryWidget.generated.h"

class UUniformGridPanel;
class UPDInventoryComponent;
class UPDStashComponent;
class UPDQuickSlotComponent;
class UPDSecureContainerComponent;
class UPDInventorySlotWidget;
class UUserWidget;
class UWidgetTree;
class UTextBlock;
class UPDQuantityPopupWidget;
class UPDInventoryItemContextMenuWidget;
class UPanelWidget;
class UPDEquipmentComponent;
class UButton;
class UWidget;
class UPDInventoryWeightBarWidget;

UCLASS(BlueprintType, Blueprintable)
class PROJECTD_API UPDInventoryWidget : public UPDActivatableBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "PD|Inventory")
	void RefreshInventoryGrid();

	UFUNCTION(BlueprintCallable, Category = "PD|Inventory")
	void SetInventoryFilterTab(EPDItemFilterTab NewFilterTab);

	UFUNCTION(BlueprintCallable, Category = "PD|Inventory")
	void SetInventorySortMode(EPDItemSortMode NewSortMode);

	UFUNCTION(BlueprintCallable, Category = "PD|Inventory")
	void SetActiveStashComponent(UPDStashComponent* InStashComponent);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Inventory")
	TSubclassOf<UUserWidget> InventorySlotWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Inventory")
	int32 FallbackGridColumns = 5;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Inventory")
	int32 FallbackGridRows = 4;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Inventory")
	FName InventoryGridWidgetName = TEXT("UniformGridPanel_InventoryGrid");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Inventory")
	FName GoldTextWidgetName = TEXT("Text_Gold");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Inventory")
	FName InventoryWeightBarWidgetName = TEXT("WBP_WeightBar");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Inventory")
	TSubclassOf<UPDQuantityPopupWidget> QuantityPopupWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Inventory")
	TSubclassOf<UPDInventoryItemContextMenuWidget> ContextMenuWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Inventory")
	bool bEnableContextMenu = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Inventory")
	FName ContextMenuContainerWidgetName = TEXT("Panel_ContextMenu");

	UPROPERTY(Transient, BlueprintReadOnly, Category = "PD|Inventory")
	TObjectPtr<UUniformGridPanel> InventoryGridPanel;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "PD|Inventory|Tabs")
	TObjectPtr<UButton> Button_Equipment;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "PD|Inventory|Tabs")
	TObjectPtr<UButton> Button_Consumable;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "PD|Inventory|Tabs")
	TObjectPtr<UButton> Button_Misc;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "PD|Inventory|Sort")
	TObjectPtr<UButton> Button_Sort;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "PD|Inventory|Sort")
	TObjectPtr<UButton> Button_SortByName;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "PD|Inventory|Sort")
	TObjectPtr<UButton> Button_SortByType;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "PD|Inventory|Sort")
	TObjectPtr<UButton> Button_SortTab_Name;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "PD|Inventory|Sort")
	TObjectPtr<UButton> Button_SortTab_Type;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "PD|Inventory|Sort")
	TObjectPtr<UWidget> Panel_SortOptions;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "PD|Inventory|Sort")
	TObjectPtr<UWidget> Panel_SortTabs;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Inventory|Equipment")
	FName EquipmentSlotWeaponWidgetName = TEXT("EquipmentSlot_Weapon");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Inventory|Equipment")
	FName EquipmentSlotHeadWidgetName = TEXT("EquipmentSlot_Head");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Inventory|Equipment")
	FName EquipmentSlotArmorWidgetName = TEXT("EquipmentSlot_Armor");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Inventory|Equipment")
	FName EquipmentSlotBagWidgetName = TEXT("EquipmentSlot_Bag");
	
	UFUNCTION()
	void HandleInventorySlotLeftClicked(UPDInventorySlotWidget* SlotWidget, int32 ClickedSlotIndex);

	UFUNCTION()
	void HandleInventorySlotRightClicked(UPDInventorySlotWidget* SlotWidget, int32 ClickedSlotIndex);

	UFUNCTION()
	void HandleInventorySlotHovered(UPDInventorySlotWidget* SlotWidget, int32 HoveredSlotIndex);

	UFUNCTION()
	void HandleInventorySlotUnhovered(UPDInventorySlotWidget* SlotWidget, int32 UnhoveredSlotIndex);

	UFUNCTION()
	void HandleInventorySlotItemDropped(UPDInventorySlotWidget* SlotWidget, int32 TargetSlotIndex, UPDInventoryDragDropOperation* DragOperation);

	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Inventory|Weight")
	void BP_OnInventoryWeightLimitExceeded(float CurrentWeight, float MaxWeight);

	UFUNCTION()
	void HandleContextMenuUseClicked(UPDInventoryItemContextMenuWidget* MenuWidget, int32 SlotIndex);

	UFUNCTION()
	void HandleContextMenuDropClicked(UPDInventoryItemContextMenuWidget* MenuWidget, int32 SlotIndex);

	UFUNCTION()
	void HandleContextMenuEquipClicked(UPDInventoryItemContextMenuWidget* MenuWidget, int32 SlotIndex);

	UFUNCTION()
	void HandleQuantityConfirmed(int32 Quantity);

	UFUNCTION()
	void HandleQuantityCancelled();

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
	void HandleEquipmentSlotRightClicked(UPDInventorySlotWidget* SlotWidget, int32 EquipmentSlotIndex);

	UFUNCTION()
	void HandleEquipmentSlotItemDropped(UPDInventorySlotWidget* SlotWidget, int32 EquipmentSlotIndex, UPDInventoryDragDropOperation* DragOperation);

private:
	void ResolveInventoryGridPanel();
	void BindTabButtons();
	void BindSortButtons();
	void UpdateTabButtonStyle();
	void ResolveEquipmentSlotWidgets();
	void BindEquipmentChanged();
	void UnbindEquipmentChanged();
	void RefreshEquipmentSlots();
	void RegisterEquipmentSlotWidget(EPDEquipmentSlotType SlotType, FName WidgetName);
	FText GetEquipmentSlotLabel(EPDEquipmentSlotType SlotType) const;
	int32 CountOccupiedInventorySlotsByType(EPDItemType ItemType) const;
	int32 GetInventoryDisplaySlotCount() const;
	void SetTabButtonLabel(UButton* TargetButton, const FText& BaseLabel, int32 UsedSlots, int32 MaxSlots) const;
	bool DoesSlotMatchCurrentFilter(const FPDInventorySlot& InventorySlotData) const;
	bool DoesItemTypeMatchCurrentFilter(EPDItemType ItemType) const;
	bool CanAcceptDropForCurrentFilter(const UPDInventoryDragDropOperation* DragOperation) const;
	void SortDisplaySlotIndices(TArray<int32>& DisplaySlotIndices, const UPDInventoryComponent* InventoryComponent) const;
	void SetSortOptionsVisible(bool bVisible);
	void ToggleSortOptions();
	void RefreshGoldText();
	void RefreshInventoryWeightBar();
	void ResolveInventoryWeightBarWidget();
	void ExecuteInventoryQuickAction(int32 SlotIndex, int32 Quantity);
	void ExecuteInventorySlotTransfer(EPDItemContainerType SourceContainerType, int32 SourceSlotIndex, int32 TargetSlotIndex, int32 Quantity);
	void OpenContextMenu(UPDInventorySlotWidget* SlotWidget, int32 SlotIndex);
	void CloseContextMenu();
	UPanelWidget* FindContextMenuContainer() const;
	void OpenItemHoverTooltip(UPDInventorySlotWidget* SlotWidget);
	void CloseItemHoverTooltip();
	FVector2D GetSlotTooltipPosition(UPDInventorySlotWidget* SlotWidget) const;
	void OpenQuantityPopup(int32 SlotIndex, int32 MaxQuantity, const FText& Title);
	void OpenTransferQuantityPopup(EPDItemContainerType SourceContainerType, int32 SourceSlotIndex, int32 TargetSlotIndex, int32 MaxQuantity, const FText& Title);
	bool ShouldOpenTransferQuantityPopup(EPDItemContainerType SourceContainerType, int32 SourceSlotIndex, int32 TargetSlotIndex, int32& OutMaxQuantity, FText& OutTitle) const;
	const FPDInventorySlot* FindSourceSlot(EPDItemContainerType SourceContainerType, int32 SlotIndex) const;
	const FPDInventorySlot* FindInventorySlot(int32 SlotIndex) const;
	void ClearQuantityRequest();
	UPDInventoryComponent* FindInventoryComponent() const;
	UPDStashComponent* FindStashComponent() const;
	UPDQuickSlotComponent* FindQuickSlotComponent() const;
	UPDSecureContainerComponent* FindSecureContainerComponent() const;
	UPDEquipmentComponent* FindEquipmentComponent() const;

	UFUNCTION()
	void HandleEquipmentChanged();

	UFUNCTION()
	void HandleInventoryWeightLimitExceeded(float CurrentWeight, float MaxWeight);

	UFUNCTION()
	void HandleInventoryMessage(const FText& Message);
	void BindInventoryChanged();
	void UnbindInventoryChanged();

	UPROPERTY(Transient)
	TObjectPtr<UPDInventoryComponent> BoundInventoryComponent;

	UPROPERTY(Transient)
	TObjectPtr<UPDEquipmentComponent> BoundEquipmentComponent;

	TMap<EPDEquipmentSlotType, TWeakObjectPtr<UPDInventorySlotWidget>> EquipmentSlotWidgets;

	UPROPERTY(Transient)
	TObjectPtr<UPDStashComponent> ActiveStashComponent;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> GoldTextWidget;

	UPROPERTY(Transient)
	TObjectPtr<UPDInventoryWeightBarWidget> InventoryWeightBarWidget;

	UPROPERTY(Transient)
	TObjectPtr<UPDQuantityPopupWidget> ActiveQuantityPopup;

	UPROPERTY(Transient)
	TObjectPtr<UPDInventoryItemContextMenuWidget> ActiveContextMenu;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> ActiveItemTooltip;

	UPROPERTY(Transient)
	TObjectPtr<UPDInventorySlotWidget> ActiveTooltipSlot;

	FVector2D ActiveTooltipPosition = FVector2D::ZeroVector;

	bool bPendingTransferQuantityRequest = false;
	EPDItemContainerType PendingTransferSourceContainerType = EPDItemContainerType::None;
	int32 PendingTransferSourceSlotIndex = INDEX_NONE;
	int32 PendingTransferTargetSlotIndex = INDEX_NONE;

	EPDItemFilterTab CurrentFilterTab = EPDItemFilterTab::Equipment;
	EPDItemSortMode CurrentSortMode = EPDItemSortMode::None;

	int32 PendingSlotIndex = INDEX_NONE;
};
