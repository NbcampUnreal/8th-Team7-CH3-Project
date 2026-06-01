#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Type/Types.h"
#include "PDEquipmentListItemWidget.generated.h"

class UImage;
class UTextBlock;
class UPDEquipmentListItemWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPDOnEquipmentListItemClicked, UPDEquipmentListItemWidget*, ItemWidget, int32, SlotIndex);

UCLASS(BlueprintType, Blueprintable)
class PROJECTD_API UPDEquipmentListItemWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "PD|Equipment")
	void SetSlotData(const FPDInventorySlot& InSlotData, int32 InSlotIndex);

	UFUNCTION(BlueprintCallable, Category = "PD|Equipment")
	void SetSelected(bool bInSelected);

	UFUNCTION(BlueprintPure, Category = "PD|Equipment")
	const FPDInventorySlot& GetSlotData() const { return SlotData; }

	UFUNCTION(BlueprintPure, Category = "PD|Equipment")
	int32 GetSlotIndex() const { return SlotIndex; }

	UPROPERTY(BlueprintAssignable, Category = "PD|Equipment|Event")
	FPDOnEquipmentListItemClicked OnItemClicked;

protected:
	virtual void NativeOnInitialized() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"), Category = "PD|Equipment|Widget")
	TObjectPtr<UImage> Image_ItemIcon;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"), Category = "PD|Equipment|Widget")
	TObjectPtr<UTextBlock> Text_ItemName;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"), Category = "PD|Equipment|Widget")
	TObjectPtr<UTextBlock> Text_Level;

private:
	void RefreshVisuals();

	UPROPERTY(Transient)
	FPDInventorySlot SlotData;

	int32 SlotIndex = INDEX_NONE;
	bool bSelected = false;
};
