#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Type/Types.h"
#include "Items/Equipment/PDEquipmentModificationComponent.h"
#include "PDEquipmentModificationWidget.generated.h"

class USoundBase;
class UButton;
class UImage;
class UScrollBox;
class UTextBlock;
class UUniformGridPanel;
class UUniformGridSlot;
class UVerticalBox;
class UPDInventoryComponent;
class UPDEquipmentListItemWidget;
class UPDInventorySlotWidget;
class UPDInventoryDragDropOperation;

UCLASS(BlueprintType, Blueprintable)
class PROJECTD_API UPDEquipmentModificationWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "PD|Equipment Modification")
	void RefreshAll();

	UFUNCTION(BlueprintCallable, Category = "PD|Equipment Modification")
	void RefreshEquipmentList();

	UFUNCTION(BlueprintCallable, Category = "PD|Equipment Modification")
	void RefreshInventoryGrid();

	UFUNCTION(BlueprintCallable, Category = "PD|Equipment Modification")
	void RefreshPreview();

	UFUNCTION(BlueprintCallable, Category = "PD|Equipment Modification")
	void SelectInventorySlot(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "PD|Equipment Modification")
	void SelectMaterialSlot(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "PD|Equipment Modification")
	void SelectBoostSlot(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "PD|Equipment Modification")
	void SetInventoryFilter(EPDItemType ItemType);

	UFUNCTION(BlueprintCallable, Category = "PD|Equipment Modification")
	void SetBoostType(EPDModificationBoostType BoostType);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|UI Sound")
	TObjectPtr<USoundBase> ButtonClickSound;

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Equipment Modification|Widget")
	TSubclassOf<UPDEquipmentListItemWidget> EquipmentListItemWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Equipment Modification|Widget")
	TSubclassOf<UPDInventorySlotWidget> InventorySlotWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Equipment Modification|Widget", meta = (ClampMin = "1"))
	int32 InventoryGridColumns = 4;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Equipment Modification|Widget", meta = (ClampMin = "1"))
	int32 InventoryGridRows = 5;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Equipment Modification|Widget", meta = (ClampMin = "0.0"))
	float InventoryGridSlotPadding = 6.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Equipment Modification|Format")
	FText EmptySelectionText = FText::FromString(TEXT("Select an item."));

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Equipment Modification|Format")
	FText EmptyMaterialText = FText::FromString(TEXT("Select material."));

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Equipment Modification|Format")
	FText ModifyReadyText = FText::FromString(TEXT("Preview will appear after modification."));

private:
	void ResolveComponents();
	void BindWidgetEvents();
	void UnbindWidgetEvents();
	void BindComponentEvents();
	void UnbindComponentEvents();
	void SelectFirstEquipmentSlotIfNeeded();
	void RefreshSelectedSlots();
	void ClearPreview();
	bool IsRegisteredBoostItem(FName ItemID) const;
	bool TryAutoSelectInventoryItem(int32 SlotIndex);
	void ApplyPreview(const FPDModificationPreview& Preview, const FPDInventorySlot& SlotData);
	void RefreshMaterialList(const FPDModificationPreview& Preview);
	void SetText(UTextBlock* TextBlock, const FText& Text) const;
	void SetSlotWidgetData(UPDInventorySlotWidget* SlotWidget, int32 SlotIndex, const FText& EmptyLabel) const;
	bool IsValidEquipmentSlotIndex(int32 SlotIndex) const;
	bool IsValidMaterialSlotIndex(int32 SlotIndex) const;
	bool DoesSelectedMaterialMatchPreview(const FPDModificationPreview& Preview) const;
	int32 CountItem(FName ItemID) const;
	FText GetResultText(EPDModificationResult Result, bool bSuccess) const;
	FText FormatPercent(float Rate) const;
	FText FormatLevelText(int32 Level) const;
	FText FormatGoldAmount(int32 Gold) const;

	UFUNCTION()
	void HandleInventoryChanged();

	UFUNCTION()
	void HandleModificationFinished(int32 InventorySlotIndex, int32 NewModificationLevel, bool bSuccess, EPDModificationResult Result);

	UFUNCTION()
	void HandleEquipmentListItemClicked(UPDEquipmentListItemWidget* ItemWidget, int32 SlotIndex);

	UFUNCTION()
	void HandleInventorySlotClicked(UPDInventorySlotWidget* SlotWidget, int32 SlotIndex);

	UFUNCTION()
	void HandleSelectionSlotItemDropped(UPDInventorySlotWidget* SlotWidget, int32 SlotIndex, UPDInventoryDragDropOperation* DragOperation);

	UFUNCTION()
	void HandleEquipmentTabClicked();

	UFUNCTION()
	void HandleMaterialTabClicked();

	UFUNCTION()
	void HandleBoostNoneClicked();

	UFUNCTION()
	void HandleBoostLowClicked();

	UFUNCTION()
	void HandleBoostMidClicked();

	UFUNCTION()
	void HandleBoostHighClicked();

	UFUNCTION()
	void HandleModifyClicked();

	UPROPERTY(Transient)
	TObjectPtr<UPDInventoryComponent> InventoryComponent;

	UPROPERTY(Transient)
	TObjectPtr<UPDEquipmentModificationComponent> ModificationComponent;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"), Category = "PD|Equipment Modification|Widget")
	TObjectPtr<UScrollBox> ScrollBox_EquipmentList;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"), Category = "PD|Equipment Modification|Widget")
	TObjectPtr<UUniformGridPanel> InventoryGrid;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"), Category = "PD|Equipment Modification|Widget")
	TObjectPtr<UPDInventorySlotWidget> EquipmentSlotWidget;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"), Category = "PD|Equipment Modification|Widget")
	TObjectPtr<UPDInventorySlotWidget> MaterialSlotWidget;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"), Category = "PD|Equipment Modification|Widget")
	TObjectPtr<UPDInventorySlotWidget> BoostSlotWidget;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"), Category = "PD|Equipment Modification|Widget")
	TObjectPtr<UPDInventorySlotWidget> BoostMaterialSlotWidget;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"), Category = "PD|Equipment Modification|Widget")
	TObjectPtr<UImage> Image_SelectedItemIcon;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"), Category = "PD|Equipment Modification|Widget")
	TObjectPtr<UTextBlock> Text_PlayerGold;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"), Category = "PD|Equipment Modification|Widget")
	TObjectPtr<UTextBlock> Text_SelectedItemName;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"), Category = "PD|Equipment Modification|Widget")
	TObjectPtr<UTextBlock> Text_CurrentLevel;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"), Category = "PD|Equipment Modification|Widget")
	TObjectPtr<UTextBlock> Text_TargetLevel;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"), Category = "PD|Equipment Modification|Widget")
	TObjectPtr<UTextBlock> Text_StatType;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"), Category = "PD|Equipment Modification|Widget")
	TObjectPtr<UTextBlock> Text_StatPreview;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"), Category = "PD|Equipment Modification|Widget")
	TObjectPtr<UTextBlock> Text_GoldCost;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"), Category = "PD|Equipment Modification|Widget")
	TObjectPtr<UTextBlock> Text_SuccessRate;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"), Category = "PD|Equipment Modification|Widget")
	TObjectPtr<UTextBlock> Text_RequiredGold;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"), Category = "PD|Equipment Modification|Widget")
	TObjectPtr<UVerticalBox> VerticalBox_RequiredMaterials;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"), Category = "PD|Equipment Modification|Widget")
	TObjectPtr<UTextBlock> Text_BaseSuccessRate;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"), Category = "PD|Equipment Modification|Widget")
	TObjectPtr<UTextBlock> Text_BoostSuccessRate;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"), Category = "PD|Equipment Modification|Widget")
	TObjectPtr<UTextBlock> Text_FinalSuccessRate;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"), Category = "PD|Equipment Modification|Widget")
	TObjectPtr<UButton> Button_EquipmentTab;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"), Category = "PD|Equipment Modification|Widget")
	TObjectPtr<UButton> Button_MaterialTab;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"), Category = "PD|Equipment Modification|Widget")
	TObjectPtr<UButton> Button_Boost_None;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"), Category = "PD|Equipment Modification|Widget")
	TObjectPtr<UButton> Button_Boost_Low;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"), Category = "PD|Equipment Modification|Widget")
	TObjectPtr<UButton> Button_Boost_Mid;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"), Category = "PD|Equipment Modification|Widget")
	TObjectPtr<UButton> Button_Boost_High;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"), Category = "PD|Equipment Modification|Widget")
	TObjectPtr<UButton> Button_Modify;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"), Category = "PD|Equipment Modification|Widget")
	TObjectPtr<UButton> BTN_Enhance;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"), Category = "PD|Equipment Modification|Widget")
	TObjectPtr<UTextBlock> Text_Result;

	int32 SelectedSlotIndex = INDEX_NONE;
	int32 SelectedMaterialSlotIndex = INDEX_NONE;
	int32 SelectedBoostSlotIndex = INDEX_NONE;
	EPDItemType CurrentInventoryFilter = EPDItemType::Equipment;
	EPDModificationBoostType SelectedBoostType = EPDModificationBoostType::None;
	FPDModificationPreview CurrentPreview;
	EPDModificationResult CurrentPreviewResult = EPDModificationResult::InvalidSlot;
	bool bCurrentPreviewValid = false;
};
