#pragma once

#include "CoreMinimal.h"
#include "Widgets/PDActivatableBase.h"
#include "Widgets/Inventory/PDInventoryDragDropOperation.h"
#include "PDQuickSlotWidget.generated.h"

class UUniformGridPanel;
class UPDInventoryComponent;
class UPDStashComponent;
class UPDQuickSlotComponent;
class UPDInventorySlotWidget;
class UUserWidget;
class UPDQuantityPopupWidget;

UCLASS(BlueprintType, Blueprintable)
class PROJECTD_API UPDQuickSlotWidget : public UPDActivatableBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "PD|QuickSlot")
	void RefreshQuickSlotGrid();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|QuickSlot")
	TSubclassOf<UUserWidget> QuickSlotWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|QuickSlot")
	int32 FallbackGridColumns = 5;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|QuickSlot")
	int32 FallbackGridRows = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|QuickSlot")
	FName QuickSlotGridWidgetName = TEXT("UniformGridPanel_QuickSlotGrid");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|QuickSlot")
	TSubclassOf<UPDQuantityPopupWidget> QuantityPopupWidgetClass;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "PD|QuickSlot")
	TObjectPtr<UUniformGridPanel> QuickSlotGridPanel;

	UFUNCTION()
	void HandleQuickSlotItemDropped(UPDInventorySlotWidget* SlotWidget, int32 TargetSlotIndex, UPDInventoryDragDropOperation* DragOperation);

	UFUNCTION()
	void HandleQuantityConfirmed(int32 Quantity);

	UFUNCTION()
	void HandleQuantityCancelled();

private:
	void ResolveQuickSlotGridPanel();
	UPDInventoryComponent* FindInventoryComponent() const;
	UPDStashComponent* FindStashComponent() const;
	UPDQuickSlotComponent* FindQuickSlotComponent() const;
	const FPDInventorySlot* FindQuickSlot(int32 SlotIndex) const;
	const FPDInventorySlot* FindSourceSlot(EPDItemContainerType SourceContainerType, int32 SlotIndex) const;
	void ExecuteQuickSlotTransfer(EPDItemContainerType SourceContainerType, int32 SourceSlotIndex, int32 TargetSlotIndex, int32 Quantity);
	void OpenTransferQuantityPopup(EPDItemContainerType SourceContainerType, int32 SourceSlotIndex, int32 TargetSlotIndex, int32 MaxQuantity, const FText& Title);
	bool ShouldOpenTransferQuantityPopup(EPDItemContainerType SourceContainerType, int32 SourceSlotIndex, int32 TargetSlotIndex, int32& OutMaxQuantity, FText& OutTitle) const;
	void ClearQuantityRequest();
	void BindQuickSlotsChanged();
	void UnbindQuickSlotsChanged();

	UPROPERTY(Transient)
	TObjectPtr<UPDQuickSlotComponent> BoundQuickSlotComponent;

	UPROPERTY(Transient)
	TObjectPtr<UPDQuantityPopupWidget> ActiveQuantityPopup;

	EPDItemContainerType PendingTransferSourceContainerType = EPDItemContainerType::None;
	int32 PendingTransferSourceSlotIndex = INDEX_NONE;
	int32 PendingTransferTargetSlotIndex = INDEX_NONE;
};
