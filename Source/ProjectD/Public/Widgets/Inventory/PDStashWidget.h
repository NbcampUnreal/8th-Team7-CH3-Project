#pragma once

#include "CoreMinimal.h"
#include "Widgets/PDActivatableBase.h"
#include "Widgets/Inventory/PDInventoryDragDropOperation.h"
#include "PDStashWidget.generated.h"

class UUniformGridPanel;
class UPDStashComponent;
class UPDInventoryComponent;
class UPDQuickSlotComponent;
class UPDInventorySlotWidget;
class UUserWidget;
class UPDQuantityPopupWidget;

UCLASS(BlueprintType, Blueprintable)
class PROJECTD_API UPDStashWidget : public UPDActivatableBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "PD|Stash")
	void RefreshStashGrid();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Stash")
	TSubclassOf<UUserWidget> StashSlotWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Stash")
	int32 FallbackGridColumns = 10;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Stash")
	int32 FallbackGridRows = 8;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Stash")
	FName StashGridWidgetName = TEXT("UniformGridPanel_StashGrid");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Stash")
	TSubclassOf<UPDQuantityPopupWidget> QuantityPopupWidgetClass;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "PD|Stash")
	TObjectPtr<UUniformGridPanel> StashGridPanel;

	UFUNCTION()
	void HandleStashSlotLeftClicked(UPDInventorySlotWidget* SlotWidget, int32 ClickedSlotIndex);

	UFUNCTION()
	void HandleStashSlotItemDropped(UPDInventorySlotWidget* SlotWidget, int32 TargetSlotIndex, UPDInventoryDragDropOperation* DragOperation);

	UFUNCTION()
	void HandleQuantityConfirmed(int32 Quantity);

	UFUNCTION()
	void HandleQuantityCancelled();

private:
	void ResolveStashGridPanel();
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

	UPROPERTY(Transient)
	TObjectPtr<UPDStashComponent> BoundStashComponent;

	UPROPERTY(Transient)
	TObjectPtr<UPDQuantityPopupWidget> ActiveQuantityPopup;

	bool bPendingTransferQuantityRequest = false;
	EPDItemContainerType PendingTransferSourceContainerType = EPDItemContainerType::None;
	int32 PendingTransferSourceSlotIndex = INDEX_NONE;
	int32 PendingTransferTargetSlotIndex = INDEX_NONE;

	int32 PendingSlotIndex = INDEX_NONE;
};
