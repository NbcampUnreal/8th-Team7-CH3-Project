#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Type/Types.h"
#include "Widgets/Inventory/PDInventoryDragDropOperation.h"
#include "PDNewQuickSlotItemWidget.generated.h"

class UBorder;
class UImage;
class UOverlay;
class USizeBox;
class UTextBlock;
class UTexture2D;
class UMaterialInterface;
class UWidgetAnimation;
class UPDQuickSlotComponent;
class UPDInventoryComponent;
class UPDStashComponent;

UCLASS(BlueprintType, Blueprintable)
class PROJECTD_API UPDNewQuickSlotItemWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="PD|QuickSlot")
	void InitializeQuickSlot(UPDQuickSlotComponent* InQuickSlotComponent, int32 InSlotIndex);

	UFUNCTION(BlueprintCallable, Category="PD|QuickSlot")
	void SetSlotData(const FPDInventorySlot& InSlotData);

	UFUNCTION(BlueprintCallable, Category="PD|QuickSlot")
	void ClearSlotData();

	UFUNCTION(BlueprintPure, Category="PD|QuickSlot")
	int32 GetSlotIndex() const { return SlotIndex; }

	UFUNCTION(BlueprintCallable, Category="PD|QuickSlot")
	void SetSelected(bool bNewSelected);

	UFUNCTION(BlueprintCallable, Category="PD|QuickSlot")
	void SetKeyBindingIcon(UTexture2D* InIcon);

	UFUNCTION(BlueprintCallable, Category="PD|QuickSlot")
	void SetCountText(const FText& InCountText);

	UFUNCTION(BlueprintCallable, Category="PD|QuickSlot")
	void SetSlotSize(FVector2D NewSize);

	UFUNCTION(BlueprintCallable, Category="PD|QuickSlot")
	void SetSlotMaterials(TSoftObjectPtr<UMaterialInterface> InBase, TSoftObjectPtr<UMaterialInterface> InSelected);

	UFUNCTION(BlueprintCallable, Category="PD|QuickSlot")
	void SetWeaponCooldownUI(bool bActive, float RemainingTime);

	UFUNCTION(BlueprintCallable, Category="PD|QuickSlot")
	void PlayCooldownReadyFlash();

	UFUNCTION(BlueprintPure, Category="PD|QuickSlot")
	bool IsSlotSelected() const { return bSelected; }

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativePreConstruct() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UBorder> SlotBackground;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UImage> Image_ItemIcon;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Quantity;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UImage> Image_SlotBG;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UOverlay> Container_AmmoLabel;
	
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UImage> Image_AmmoCapsule;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_AmmoOrCount;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UImage> Image_KeyBinding;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UImage> Image_CooldownOverlay;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_CooldownRemain;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UImage> Image_Flash;

	UPROPERTY(Transient, BlueprintReadOnly, meta=(BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> Anim_CooldownReadyFlash;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<USizeBox> Box_Slot;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|QuickSlot|Widget")
	TSoftObjectPtr<UMaterialInterface> SlotBGMaterial_Base;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|QuickSlot|Widget")
	TSoftObjectPtr<UMaterialInterface> SlotBGMaterial_Selected;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="PD|QuickSlot")
	FVector2D SlotSize = FVector2D(72.f, 72.f);

private:
	void BuildFallbackWidget();
	void RefreshVisuals();
	void ClearWeaponCooldownUI();
	UPDInventoryComponent* FindInventoryComponent() const;
	UPDStashComponent* FindStashComponent() const;

	UPROPERTY(Transient)
	TObjectPtr<UPDQuickSlotComponent> QuickSlotComponent;

	FPDInventorySlot SlotData;
	int32 SlotIndex = INDEX_NONE;
	bool bSelected = false;
};
