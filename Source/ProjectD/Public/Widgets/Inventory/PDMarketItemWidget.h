#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Items/PDMarketComponent.h"
#include "PDMarketItemWidget.generated.h"

class UButton;
class UImage;
class UPanelWidget;
class UTextBlock;
class UPDInventoryComponent;
class UPDInventorySlotWidget;
class UPDMarketComponent;
class UPDMarketQuantityPopupWidget;

UCLASS(BlueprintType, Blueprintable)
class PROJECTD_API UPDMarketItemWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "PD|Market")
	void SetMarketEntry(UPDMarketComponent* InMarketComponent, UPDInventoryComponent* InBuyerInventory, const FPDMarketEntry& InEntry, int32 InEntryIndex);

	UFUNCTION(BlueprintCallable, Category = "PD|Market")
	void SetInventorySlotWidgetClass(TSubclassOf<UPDInventorySlotWidget> InSlotWidgetClass);

	UFUNCTION(BlueprintCallable, Category = "PD|Market")
	void SetQuantityPopupWidgetClass(TSubclassOf<UPDMarketQuantityPopupWidget> InPopupWidgetClass);

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Market|Widget", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UPDInventorySlotWidget> InventorySlotWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Market|Widget", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UPDMarketQuantityPopupWidget> QuantityPopupWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Market|Widget", meta = (AllowPrivateAccess = "true"))
	FName InventorySlotWidgetName = TEXT("WBP_InventorySlot");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Market|Widget", meta = (AllowPrivateAccess = "true"))
	FName InventorySlotHostWidgetName = TEXT("Overlay_ItemSlot");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Market|Widget", meta = (AllowPrivateAccess = "true"))
	FName TextItemNameWidgetName = TEXT("Text_ItemName");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Market|Widget", meta = (AllowPrivateAccess = "true"))
	FName TextPriceWidgetName = TEXT("Text_ItemPrice");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Market|Widget", meta = (AllowPrivateAccess = "true"))
	FName TextStockWidgetName = TEXT("Text_Stock");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Market|Widget", meta = (AllowPrivateAccess = "true"))
	FName ButtonBuyWidgetName = TEXT("Button_Buy");

private:
	UFUNCTION()
	void HandleBuyClicked();

	void ResolveWidgets();
	void RefreshVisuals();
	void RefreshInventorySlot(const FPDItemData& ItemData);
	FPDInventorySlot MakeMarketSlotData(const FPDItemData& ItemData) const;
	int32 GetUnitPrice() const;
	bool CanBuyCurrentEntry() const;
	FText MakePriceText(int32 Price) const;

	UPROPERTY(Transient)
	TObjectPtr<UPDMarketComponent> MarketComponent;

	UPROPERTY(Transient)
	TObjectPtr<UPDInventoryComponent> BuyerInventory;

	UPROPERTY(Transient)
	TObjectPtr<UPDInventorySlotWidget> InventorySlotWidget;

	UPROPERTY(Transient)
	TObjectPtr<UPanelWidget> InventorySlotHostWidget;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TextItemNameWidget;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TextPriceWidget;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TextStockWidget;

	UPROPERTY(Transient)
	TObjectPtr<UButton> ButtonBuyWidget;

	FPDMarketEntry Entry;
	int32 EntryIndex = INDEX_NONE;
};
