#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Items/Market/PDMarketComponent.h"
#include "PDMarketQuantityPopupWidget.generated.h"

class USoundBase;
class UButton;
class UImage;
class UTextBlock;
class UPDInventoryComponent;
class UPDInventorySlotWidget;

UCLASS(BlueprintType, Blueprintable)
class PROJECTD_API UPDMarketQuantityPopupWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "PD|Market")
	void InitializePurchasePopup(UPDMarketComponent* InMarketComponent, UPDInventoryComponent* InBuyerInventory, const FPDMarketEntry& InEntry, int32 InEntryIndex);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|UI Sound")
	TObjectPtr<USoundBase> ButtonClickSound;

	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Market|Widget")
	FName TextTitleWidgetName = TEXT("Text_Title");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Market|Widget")
	FName ButtonCloseWidgetName = TEXT("Button_Close");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Market|Widget")
	FName InventorySlotWidgetName = TEXT("WBP_InventorySlot");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Market|Widget")
	FName ImageItemIconWidgetName = TEXT("Image_ItemIcon");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Market|Widget")
	FName TextItemNameWidgetName = TEXT("Text_ItemName");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Market|Widget")
	FName TextItemDescWidgetName = TEXT("Text_ItemDesc");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Market|Widget")
	FName TextPriceWidgetName = TEXT("Text_Price");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Market|Widget")
	FName TextGoldWidgetName = TEXT("Text_Gold");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Market|Widget")
	FName TextQuantityWidgetName = TEXT("Text_Quantity");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Market|Widget")
	FName TextHaveCountWidgetName = TEXT("Text_HaveCount");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Market|Widget")
	FName TextTotalPriceWidgetName = TEXT("Text_TotalPrice");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Market|Widget")
	FName TextBuyPriceWidgetName = TEXT("Text_BuyPrice");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Market|Widget")
	FName ButtonMinusWidgetName = TEXT("Button_Minus");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Market|Widget")
	FName ButtonPlusWidgetName = TEXT("Button_Plus");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Market|Widget")
	FName ButtonMaxWidgetName = TEXT("Button_Max");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Market|Widget")
	FName ButtonCancelWidgetName = TEXT("Button_Cancel");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Market|Widget")
	FName ButtonBuyWidgetName = TEXT("Button_Buy");

private:
	UFUNCTION()
	void HandleMinusClicked();

	UFUNCTION()
	void HandlePlusClicked();

	UFUNCTION()
	void HandleMaxClicked();

	UFUNCTION()
	void HandleCancelClicked();

	UFUNCTION()
	void HandleBuyClicked();

	void ResolveWidgets();
	void ResolveButton(TObjectPtr<UButton>& Button, FName WidgetName);
	void RefreshVisuals();
	void RefreshBuyState();
	void SetQuantity(int32 NewQuantity);
	int32 GetMaxBuyQuantity() const;
	int32 GetCurrentOwnedCount() const;
	int32 GetUnitPrice() const;
	int32 GetTotalPrice() const;
	bool CanBuy() const;
	FText MakeGoldText(int32 Gold) const;

	UPROPERTY(Transient)
	TObjectPtr<UPDMarketComponent> MarketComponent;

	UPROPERTY(Transient)
	TObjectPtr<UPDInventoryComponent> BuyerInventory;

	UPROPERTY(Transient)
	TObjectPtr<UPDInventorySlotWidget> InventorySlotWidget;

	UPROPERTY(Transient)
	TObjectPtr<UImage> ImageItemIconWidget;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TextTitleWidget;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TextItemNameWidget;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TextItemDescWidget;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TextPriceWidget;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TextGoldWidget;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TextQuantityWidget;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TextHaveCountWidget;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TextTotalPriceWidget;

	UPROPERTY(Transient)
	TObjectPtr<UButton> ButtonCloseWidget;

	UPROPERTY(Transient)
	TObjectPtr<UButton> ButtonMinusWidget;

	UPROPERTY(Transient)
	TObjectPtr<UButton> ButtonPlusWidget;

	UPROPERTY(Transient)
	TObjectPtr<UButton> ButtonMaxWidget;

	UPROPERTY(Transient)
	TObjectPtr<UButton> ButtonCancelWidget;

	UPROPERTY(Transient)
	TObjectPtr<UButton> ButtonBuyWidget;

	FPDMarketEntry Entry;
	FPDItemData ItemData;
	int32 EntryIndex = INDEX_NONE;
	int32 Quantity = 1;
};
