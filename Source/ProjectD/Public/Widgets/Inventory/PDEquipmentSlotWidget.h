#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Type/Types.h"
#include "PDEquipmentSlotWidget.generated.h"

class UImage;
class UTextBlock;
class UPDEquipmentSlotWidget;
class UPDInventoryDragDropOperation;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPDOnEquipmentSlotRightClicked, UPDEquipmentSlotWidget*, SlotWidget, EPDEquipmentSlotType, SlotType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FPDOnEquipmentSlotItemDropped, UPDEquipmentSlotWidget*, SlotWidget, EPDEquipmentSlotType, SlotType, UPDInventoryDragDropOperation*, DragOperation);

UCLASS(BlueprintType, Blueprintable)
class PROJECTD_API UPDEquipmentSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "PD|Equipment|Slot")
	void InitializeEquipmentSlot(EPDEquipmentSlotType InSlotType);

	UFUNCTION(BlueprintCallable, Category = "PD|Equipment|Slot")
	void SetEquippedItem(const FPDInventorySlot& InItemSlot);

	UFUNCTION(BlueprintCallable, Category = "PD|Equipment|Slot")
	void ClearEquippedItem();

	UFUNCTION(BlueprintPure, Category = "PD|Equipment|Slot")
	EPDEquipmentSlotType GetSlotType() const { return SlotType; }

	UFUNCTION(BlueprintPure, Category = "PD|Equipment|Slot")
	const FPDInventorySlot& GetEquippedItem() const { return EquippedItem; }

	UPROPERTY(BlueprintAssignable, Category = "PD|Equipment|Slot|Event")
	FPDOnEquipmentSlotRightClicked OnEquipmentSlotRightClicked;

	UPROPERTY(BlueprintAssignable, Category = "PD|Equipment|Slot|Event")
	FPDOnEquipmentSlotItemDropped OnEquipmentSlotItemDropped;

protected:
	virtual void NativeOnInitialized() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	UPROPERTY(BlueprintReadOnly, Category = "PD|Equipment|Slot")
	EPDEquipmentSlotType SlotType = EPDEquipmentSlotType::None;

	UPROPERTY(BlueprintReadOnly, Category = "PD|Equipment|Slot")
	FPDInventorySlot EquippedItem;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Equipment|Widget", meta = (AllowPrivateAccess = "true"))
	FName TextItemNameWidgetName = TEXT("Text_ItemName");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Equipment|Widget", meta = (AllowPrivateAccess = "true"))
	FName TextSlotNameWidgetName = TEXT("Text_SlotLabel");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Equipment|Widget", meta = (AllowPrivateAccess = "true"))
	FName ImageItemIconWidgetName = TEXT("Image_ItemIcon");

private:
	void ResolveWidgets();
	void RefreshVisuals();
	FText GetDefaultSlotLabel() const;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TextItemNameWidget;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TextSlotNameWidget;

	UPROPERTY(Transient)
	TObjectPtr<UImage> ImageItemIconWidget;
};
