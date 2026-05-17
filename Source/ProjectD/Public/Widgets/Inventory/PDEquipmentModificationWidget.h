#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Type/Types.h"
#include "Items/PDEquipmentModificationComponent.h"
#include "PDEquipmentModificationWidget.generated.h"

class UButton;
class UImage;
class UScrollBox;
class UTextBlock;
class UVerticalBox;
class UPDInventoryComponent;
class UPDEquipmentListItemWidget;

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
	void RefreshPreview();

	UFUNCTION(BlueprintCallable, Category = "PD|Equipment Modification")
	void SelectInventorySlot(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "PD|Equipment Modification")
	void SetBoostType(EPDModificationBoostType BoostType);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Equipment Modification|Widget")
	TSubclassOf<UPDEquipmentListItemWidget> EquipmentListItemWidgetClass;



















	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Equipment Modification|Format")
	FText EmptySelectionText = FText::FromString(TEXT("장비를 선택하세요"));

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Equipment Modification|Format")
	FText ModifyReadyText = FText::FromString(TEXT("개조를 시도하면 결과가 표시됩니다."));

private:
	void ResolveComponents();
	void BindWidgetEvents();
	void UnbindWidgetEvents();
	void BindComponentEvents();
	void UnbindComponentEvents();
	void SelectFirstEquipmentSlotIfNeeded();
	void ClearPreview();
	void ApplyPreview(const FPDModificationPreview& Preview, const FPDInventorySlot& SlotData);
	void RefreshMaterialList(const FPDModificationPreview& Preview);
	void SetText(UTextBlock* TextBlock, const FText& Text) const;
	int32 CountItem(FName ItemID) const;
	FText GetResultText(EPDModificationResult Result, bool bSuccess) const;
	FText FormatPercent(float Rate) const;
	FText FormatLevelText(int32 Level) const;

	UFUNCTION()
	void HandleInventoryChanged();

	UFUNCTION()
	void HandleModificationFinished(int32 InventorySlotIndex, int32 NewModificationLevel, bool bSuccess, EPDModificationResult Result);

	UFUNCTION()
	void HandleEquipmentListItemClicked(UPDEquipmentListItemWidget* ItemWidget, int32 SlotIndex);

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
	TObjectPtr<UImage> Image_SelectedItemIcon;

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
	TObjectPtr<UVerticalBox> VerticalBox_RequiredMaterials;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"), Category = "PD|Equipment Modification|Widget")
	TObjectPtr<UTextBlock> Text_BaseSuccessRate;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"), Category = "PD|Equipment Modification|Widget")
	TObjectPtr<UTextBlock> Text_BoostSuccessRate;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"), Category = "PD|Equipment Modification|Widget")
	TObjectPtr<UTextBlock> Text_FinalSuccessRate;

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
	TObjectPtr<UTextBlock> Text_Result;

	int32 SelectedSlotIndex = INDEX_NONE;
	EPDModificationBoostType SelectedBoostType = EPDModificationBoostType::None;
	FPDModificationPreview CurrentPreview;
	EPDModificationResult CurrentPreviewResult = EPDModificationResult::InvalidSlot;
	bool bCurrentPreviewValid = false;
};
