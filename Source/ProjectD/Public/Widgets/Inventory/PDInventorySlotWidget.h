#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Type/Types.h"
#include "Widgets/Inventory/PDInventoryDragDropOperation.h"
#include "PDInventorySlotWidget.generated.h"

class UTextBlock;
class UImage;
class UBorder;
class UMaterialInterface;
class UPDInventorySlotWidget;
class UPDItemGradeColorData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPDOnInventorySlotClicked, UPDInventorySlotWidget*, SlotWidget, int32, SlotIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPDOnInventorySlotHovered, UPDInventorySlotWidget*, SlotWidget, int32, SlotIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FPDOnInventorySlotItemDropped, UPDInventorySlotWidget*, SlotWidget, int32, SlotIndex, UPDInventoryDragDropOperation*, DragOperation);

UCLASS(BlueprintType, Blueprintable)
class PROJECTD_API UPDInventorySlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "PD|Inventory")
	void SetSlotData(const FPDInventorySlot& InSlotData, int32 InSlotIndex);

	UFUNCTION(BlueprintCallable, Category = "PD|Inventory")	
	void ClearSlotData(int32 InSlotIndex);

	UFUNCTION(BlueprintCallable, Category = "PD|Inventory")
	void SetSlotContainerType(EPDItemContainerType InSlotContainerType);

	UFUNCTION(BlueprintPure, Category = "PD|Inventory")
	EPDItemContainerType GetSlotContainerType() const { return SlotContainerType; }

	UFUNCTION(BlueprintPure, Category = "PD|Inventory")
	const FPDInventorySlot& GetSlotData() const { return SlotData; }

	UFUNCTION(BlueprintPure, Category = "PD|Inventory")
	int32 GetSlotIndex() const { return SlotIndex; }

	UFUNCTION(BlueprintPure, Category = "PD|Inventory")
	bool WasLastClickWithControl() const { return bLastClickWithControl; }

	UFUNCTION(BlueprintCallable, Category = "PD|Inventory|Tooltip")
	UUserWidget* CreateItemTooltipWidget();

	UPROPERTY(BlueprintAssignable, Category = "PD|Inventory|Event")
	FPDOnInventorySlotClicked OnSlotLeftClicked;

	UPROPERTY(BlueprintAssignable, Category = "PD|Inventory|Event")
	FPDOnInventorySlotClicked OnSlotRightClicked;

	UPROPERTY(BlueprintAssignable, Category = "PD|Inventory|Event")
	FPDOnInventorySlotHovered OnSlotHovered;

	UPROPERTY(BlueprintAssignable, Category = "PD|Inventory|Event")
	FPDOnInventorySlotHovered OnSlotUnhovered;

	UPROPERTY(BlueprintAssignable, Category = "PD|Inventory|Event")
	FPDOnInventorySlotItemDropped OnSlotItemDropped;

	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Inventory|Event")
	void OpenItemDropdown();

	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Inventory|Event")
	void OpenUseDropMenu();

protected:
	virtual void NativeOnInitialized() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	/* --- 추가 Start--- */
	virtual bool NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	virtual bool CanAcceptDrop(UDragDropOperation* InOperation) const;
	/* --- 추가 End--- */

	UPROPERTY(BlueprintReadOnly, Category = "PD|Inventory")
	FPDInventorySlot SlotData;

	UPROPERTY(BlueprintReadOnly, Category = "PD|Inventory")
	int32 SlotIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "PD|Inventory")
	EPDItemContainerType SlotContainerType = EPDItemContainerType::None;

	UPROPERTY(BlueprintReadOnly, Category = "PD|Inventory")
	bool bLastClickWithControl = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Inventory|Debug")
	bool bShowDebugSlotIndex = false;

	/* --- 추가 Start--- */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Inventory|Widget", meta = (AllowPrivateAccess = "true"))
	TSoftObjectPtr<UMaterialInterface> SlotBGMaterial_Empty;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Inventory|Widget", meta = (AllowPrivateAccess = "true"))
	TSoftObjectPtr<UMaterialInterface> SlotBGMaterial_Filled;

	/** 등급별 슬롯 배경 틴트. 비어있으면 흰색(원본 머티리얼 그대로). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Inventory|Widget", meta = (AllowPrivateAccess = "true"))
	TSoftObjectPtr<UPDItemGradeColorData> GradeColorData;
	/* --- 추가 End--- */

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Inventory|Tooltip", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UUserWidget> TooltipWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Inventory|Tooltip", meta = (AllowPrivateAccess = "true"))
	bool bUseNativeMouseFollowingTooltip = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Inventory|Tooltip", meta = (AllowPrivateAccess = "true"))
	FName TooltipItemNameWidgetName = TEXT("Text_ItemName");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Inventory|Tooltip", meta = (AllowPrivateAccess = "true"))
	FName TooltipDescriptionWidgetName = TEXT("Text_Description");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Inventory|Tooltip", meta = (AllowPrivateAccess = "true"))
	FName TooltipWeightWidgetName = TEXT("Text_Weight");

private:
	void ClearDropOverlays();
	void RefreshVisuals();
	void ApplyTooltip(const FText& DisplayName, const FText& Description);
	void ClearTooltip();
	void ApplyTooltipTextToWidget(UUserWidget* TooltipWidget, const FText& DisplayName, const FText& Description) const;
	UPDInventorySlotWidget* CreateDragVisualWidget() const;

	FText CachedTooltipDisplayName;
	FText CachedTooltipDescription;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true"))
	TObjectPtr<UTextBlock> Text_ItemName = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true"))
	TObjectPtr<UTextBlock> Text_Quantity = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true"))
	TObjectPtr<UImage> Image_ItemIcon = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true"))
	TObjectPtr<UBorder> Border_SlotBG = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UImage> Image_HoverBorder = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UImage> Image_DropValid = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UImage> Image_DropInvalid = nullptr;
};
