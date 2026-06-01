#pragma once

#include "CoreMinimal.h"
#include "Widgets/PDActivatableBase.h"
#include "Type/Types.h"
#include "PDMarketWidget.generated.h"

class UPanelWidget;
class UPDMarketComponent;
class UPDInventoryComponent;
class UPDInventorySlotWidget;
class UPDMarketItemWidget;
class UPDMarketQuantityPopupWidget;
class UTextBlock;

UCLASS(BlueprintType, Blueprintable)
class PROJECTD_API UPDMarketWidget : public UPDActivatableBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "PD|Market")
	void InitializeMarket(UPDMarketComponent* InMarketComponent);

	UFUNCTION(BlueprintCallable, Category = "PD|Market")
	void RefreshMarketGoods();

	UFUNCTION(BlueprintCallable, Category = "PD|Market")
	void RefreshMarketInfo();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Market")
	TSubclassOf<UPDInventorySlotWidget> MarketSlotWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Market")
	TSubclassOf<UPDMarketItemWidget> MarketRowWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Market")
	TSubclassOf<UPDMarketQuantityPopupWidget> MarketQuantityPopupWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Market")
	FName MarketListWidgetName = TEXT("ScrollBox_MarketGoods");


	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Market")
	FName InventoryGoldTextWidgetName = TEXT("Text_Gold");

private:
	void ResolveMarketListPanel();
	void ResolveMarketInfoTextBlocks();
	void BindMarketChanged();
	void UnbindMarketChanged();
	void BindInventoryChanged();
	void UnbindInventoryChanged();
	void RefreshInventoryGold();
	UPDInventoryComponent* FindInventoryComponent() const;
	FText MakeGoldText(int32 Gold) const;



	UPROPERTY(Transient)
	TObjectPtr<UPanelWidget> MarketListPanel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TextInventoryGold;

	UPROPERTY(Transient)
	TObjectPtr<UPDMarketComponent> MarketComponent;

	UPROPERTY(Transient)
	TObjectPtr<UPDMarketComponent> BoundMarketComponent;

	UPROPERTY(Transient)
	TObjectPtr<UPDInventoryComponent> BoundInventoryComponent;
};
