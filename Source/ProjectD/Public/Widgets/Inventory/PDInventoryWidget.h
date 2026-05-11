
#pragma once

#include "CoreMinimal.h"
#include "Widgets/PDActivatableBase.h"
#include "PDInventoryWidget.generated.h"

class UUniformGridPanel;
class UPDInventoryComponent;
class UPDStashComponent;
class UPDInventorySlotWidget;
class UUserWidget;
class UWidgetTree;
class UTextBlock;
class UPDQuantityPopupWidget;
class UPDInventoryItemContextMenuWidget;
class UPanelWidget;

UCLASS(BlueprintType, Blueprintable)
class PROJECTD_API UPDInventoryWidget : public UPDActivatableBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "PD|Inventory")
	void RefreshInventoryGrid();

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
	TSubclassOf<UPDQuantityPopupWidget> QuantityPopupWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Inventory")
	TSubclassOf<UPDInventoryItemContextMenuWidget> ContextMenuWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Inventory")
	bool bEnableContextMenu = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Inventory")
	FName ContextMenuContainerWidgetName = TEXT("Panel_ContextMenu");

	UPROPERTY(Transient, BlueprintReadOnly, Category = "PD|Inventory")
	TObjectPtr<UUniformGridPanel> InventoryGridPanel;
	
	UFUNCTION()
	void HandleInventorySlotLeftClicked(UPDInventorySlotWidget* SlotWidget, int32 ClickedSlotIndex);

	UFUNCTION()
	void HandleInventorySlotRightClicked(UPDInventorySlotWidget* SlotWidget, int32 ClickedSlotIndex);

	UFUNCTION()
	void HandleInventorySlotHovered(UPDInventorySlotWidget* SlotWidget, int32 HoveredSlotIndex);

	UFUNCTION()
	void HandleInventorySlotUnhovered(UPDInventorySlotWidget* SlotWidget, int32 UnhoveredSlotIndex);

	UFUNCTION()
	void HandleContextMenuUseClicked(UPDInventoryItemContextMenuWidget* MenuWidget, int32 SlotIndex);

	UFUNCTION()
	void HandleContextMenuDropClicked(UPDInventoryItemContextMenuWidget* MenuWidget, int32 SlotIndex);

	UFUNCTION()
	void HandleContextMenuEquipClicked(UPDInventoryItemContextMenuWidget* MenuWidget, int32 SlotIndex);

	UFUNCTION()
	void HandleQuantityConfirmed(int32 Quantity);

private:
	void ResolveInventoryGridPanel();
	void RefreshGoldText();
	void ExecuteInventoryQuickAction(int32 SlotIndex, int32 Quantity);
	void OpenContextMenu(UPDInventorySlotWidget* SlotWidget, int32 SlotIndex);
	void CloseContextMenu();
	UPanelWidget* FindContextMenuContainer() const;
	void OpenItemHoverTooltip(UPDInventorySlotWidget* SlotWidget);
	void CloseItemHoverTooltip();
	FVector2D GetSlotTooltipPosition(UPDInventorySlotWidget* SlotWidget) const;
	void OpenQuantityPopup(int32 SlotIndex, int32 MaxQuantity, const FText& Title);
	UPDInventoryComponent* FindInventoryComponent() const;
	UPDStashComponent* FindStashComponent() const;
	void BindInventoryChanged();
	void UnbindInventoryChanged();

	UPROPERTY(Transient)
	TObjectPtr<UPDInventoryComponent> BoundInventoryComponent;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> GoldTextWidget;

	UPROPERTY(Transient)
	TObjectPtr<UPDQuantityPopupWidget> ActiveQuantityPopup;

	UPROPERTY(Transient)
	TObjectPtr<UPDInventoryItemContextMenuWidget> ActiveContextMenu;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> ActiveItemTooltip;

	UPROPERTY(Transient)
	TObjectPtr<UPDInventorySlotWidget> ActiveTooltipSlot;

	FVector2D ActiveTooltipPosition = FVector2D::ZeroVector;

	int32 PendingSlotIndex = INDEX_NONE;
};
