#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Items/PDMarketComponent.h"
#include "PDMarketItemWidget.generated.h"

class UImage;
class UTextBlock;
class UButton;
class UPDInventoryComponent;
class UPDMarketComponent;

UCLASS(BlueprintType, Blueprintable)
class PROJECTD_API UPDMarketItemWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "PD|Market")
	void SetMarketEntry(UPDMarketComponent* InMarketComponent, UPDInventoryComponent* InBuyerInventory, const FPDMarketEntry& InEntry, int32 InEntryIndex);

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Market|Widget", meta = (AllowPrivateAccess = "true"))
	FName ImageItemIconWidgetName = TEXT("Image_ItemIcon");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Market|Widget", meta = (AllowPrivateAccess = "true"))
	FName TextItemNameWidgetName = TEXT("Text_ItemName");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Market|Widget", meta = (AllowPrivateAccess = "true"))
	FName TextPriceWidgetName = TEXT("Text_Price");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Market|Widget", meta = (AllowPrivateAccess = "true"))
	FName TextStockWidgetName = TEXT("Text_Stock");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Market|Widget", meta = (AllowPrivateAccess = "true"))
	FName ButtonBuyWidgetName = TEXT("Button_Buy");

private:
	UFUNCTION()
	void HandleBuyClicked();

	void ResolveWidgets();
	void RefreshVisuals();
	int32 GetUnitPrice() const;

	UPROPERTY(Transient)
	TObjectPtr<UPDMarketComponent> MarketComponent;

	UPROPERTY(Transient)
	TObjectPtr<UPDInventoryComponent> BuyerInventory;

	UPROPERTY(Transient)
	TObjectPtr<UImage> ImageItemIconWidget;

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
