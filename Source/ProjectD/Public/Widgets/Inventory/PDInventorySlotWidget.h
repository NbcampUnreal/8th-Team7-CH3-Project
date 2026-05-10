#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Type/Types.h"
#include "PDInventorySlotWidget.generated.h"

class UTextBlock;
class UImage;
class UPDInventorySlotWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPDOnInventorySlotClicked, UPDInventorySlotWidget*, SlotWidget, int32, SlotIndex);

UCLASS(BlueprintType, Blueprintable)
class PROJECTD_API UPDInventorySlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "PD|Inventory")
	void SetSlotData(const FPDInventorySlot& InSlotData, int32 InSlotIndex);

	UFUNCTION(BlueprintCallable, Category = "PD|Inventory")	
	void ClearSlotData(int32 InSlotIndex);

	UFUNCTION(BlueprintPure, Category = "PD|Inventory")
	const FPDInventorySlot& GetSlotData() const { return SlotData; }

	UFUNCTION(BlueprintPure, Category = "PD|Inventory")
	int32 GetSlotIndex() const { return SlotIndex; }

	UFUNCTION(BlueprintPure, Category = "PD|Inventory")
	bool WasLastClickWithControl() const { return bLastClickWithControl; }

	UPROPERTY(BlueprintAssignable, Category = "PD|Inventory|Event")
	FPDOnInventorySlotClicked OnSlotLeftClicked;

	UPROPERTY(BlueprintAssignable, Category = "PD|Inventory|Event")
	FPDOnInventorySlotClicked OnSlotRightClicked;

	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Inventory|Event")
	void OpenItemDropdown();

	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Inventory|Event")
	void OpenUseDropMenu();

protected:
	virtual void NativeOnInitialized() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	UPROPERTY(BlueprintReadOnly, Category = "PD|Inventory")
	FPDInventorySlot SlotData;

	UPROPERTY(BlueprintReadOnly, Category = "PD|Inventory")
	int32 SlotIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "PD|Inventory")
	bool bLastClickWithControl = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Inventory|Debug")
	bool bShowDebugSlotIndex = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Inventory|Widget", meta = (AllowPrivateAccess = "true"))
	FName TextItemNameWidgetName = TEXT("Text_ItemName");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Inventory|Widget", meta = (AllowPrivateAccess = "true"))
	FName TextQuantityWidgetName = TEXT("Text_Quantity");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Inventory|Widget", meta = (AllowPrivateAccess = "true"))
	FName ImageItemIconWidgetName = TEXT("Image_ItemIcon");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Inventory|Tooltip", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UUserWidget> TooltipWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Inventory|Tooltip", meta = (AllowPrivateAccess = "true"))
	FName TooltipItemNameWidgetName = TEXT("Text_ItemName");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Inventory|Tooltip", meta = (AllowPrivateAccess = "true"))
	FName TooltipDescriptionWidgetName = TEXT("Text_Description");

private:
	void ResolveTextWidgets();
	void RefreshVisuals();
	void ApplyTooltip(const FText& DisplayName, const FText& Description);
	void ClearTooltip();

	TObjectPtr<UTextBlock> TextItemNameWidget = nullptr;
	TObjectPtr<UTextBlock> TextQuantityWidget = nullptr;
	TObjectPtr<UImage> ImageItemIconWidget = nullptr;
};
