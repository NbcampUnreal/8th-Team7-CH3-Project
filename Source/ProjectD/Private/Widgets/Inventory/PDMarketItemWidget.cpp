#include "Widgets/Inventory/PDMarketItemWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Items/PDInventoryComponent.h"
#include "Widgets/Inventory/PDInventorySlotWidget.h"
#include "Widgets/Inventory/PDMarketQuantityPopupWidget.h"

void UPDMarketItemWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	ResolveWidgets();
}

void UPDMarketItemWidget::SetMarketEntry(UPDMarketComponent* InMarketComponent, UPDInventoryComponent* InBuyerInventory, const FPDMarketEntry& InEntry, int32 InEntryIndex)
{
	MarketComponent = InMarketComponent;
	BuyerInventory = InBuyerInventory;
	Entry = InEntry;
	EntryIndex = InEntryIndex;
	ResolveWidgets();
	RefreshVisuals();
}

void UPDMarketItemWidget::SetInventorySlotWidgetClass(TSubclassOf<UPDInventorySlotWidget> InSlotWidgetClass)
{
	InventorySlotWidgetClass = InSlotWidgetClass;
	InventorySlotWidget = nullptr;
	RefreshVisuals();
}

void UPDMarketItemWidget::SetQuantityPopupWidgetClass(TSubclassOf<UPDMarketQuantityPopupWidget> InPopupWidgetClass)
{
	QuantityPopupWidgetClass = InPopupWidgetClass;
}

void UPDMarketItemWidget::ResolveWidgets()
{
	if (!WidgetTree)
	{
		return;
	}

	if (!InventorySlotWidget && !InventorySlotWidgetName.IsNone())
	{
		InventorySlotWidget = Cast<UPDInventorySlotWidget>(WidgetTree->FindWidget(InventorySlotWidgetName));
	}

	if (!InventorySlotHostWidget && !InventorySlotHostWidgetName.IsNone())
	{
		InventorySlotHostWidget = Cast<UPanelWidget>(WidgetTree->FindWidget(InventorySlotHostWidgetName));
	}

	if (!InventorySlotHostWidget)
	{
		InventorySlotHostWidget = Cast<UPanelWidget>(WidgetTree->FindWidget(TEXT("Panel_ItemSlot")));
	}

	if (!InventorySlotWidget && InventorySlotHostWidget && InventorySlotWidgetClass)
	{
		InventorySlotWidget = CreateWidget<UPDInventorySlotWidget>(GetOwningPlayer(), InventorySlotWidgetClass);
		if (InventorySlotWidget)
		{
			InventorySlotHostWidget->ClearChildren();
			InventorySlotHostWidget->AddChild(InventorySlotWidget);
		}
	}

	if (!TextItemNameWidget && !TextItemNameWidgetName.IsNone())
	{
		TextItemNameWidget = Cast<UTextBlock>(WidgetTree->FindWidget(TextItemNameWidgetName));
	}

	if (!TextPriceWidget && !TextPriceWidgetName.IsNone())
	{
		TextPriceWidget = Cast<UTextBlock>(WidgetTree->FindWidget(TextPriceWidgetName));
	}

	if (!TextPriceWidget)
	{
		TextPriceWidget = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("Text_Price")));
	}

	if (!TextStockWidget && !TextStockWidgetName.IsNone())
	{
		TextStockWidget = Cast<UTextBlock>(WidgetTree->FindWidget(TextStockWidgetName));
	}

	if (!ButtonBuyWidget && !ButtonBuyWidgetName.IsNone())
	{
		ButtonBuyWidget = Cast<UButton>(WidgetTree->FindWidget(ButtonBuyWidgetName));
		if (ButtonBuyWidget)
		{
			ButtonBuyWidget->OnClicked.RemoveDynamic(this, &UPDMarketItemWidget::HandleBuyClicked);
			ButtonBuyWidget->OnClicked.AddUniqueDynamic(this, &UPDMarketItemWidget::HandleBuyClicked);
		}
	}
}

void UPDMarketItemWidget::RefreshVisuals()
{
	ResolveWidgets();

	FPDItemData ItemData;
	const bool bHasItem = MarketComponent && MarketComponent->ResolveEntryItemData(Entry, ItemData);

	RefreshInventorySlot(ItemData);

	if (TextItemNameWidget)
	{
		const FText DisplayName = bHasItem && !ItemData.DisplayName.IsEmpty() ? ItemData.DisplayName : FText::FromName(Entry.ItemRowName);
		TextItemNameWidget->SetText(DisplayName);
	}

	if (TextPriceWidget)
	{
		TextPriceWidget->SetText(MakePriceText(GetUnitPrice()));
	}

	if (TextStockWidget)
	{
		TextStockWidget->SetText(Entry.Stock < 0 ? FText::FromString(TEXT("∞")) : FText::AsNumber(Entry.Stock));
	}

	if (ButtonBuyWidget)
	{
		ButtonBuyWidget->SetIsEnabled(CanBuyCurrentEntry());
	}
}

void UPDMarketItemWidget::RefreshInventorySlot(const FPDItemData& ItemData)
{
	if (!InventorySlotWidget)
	{
		return;
	}

	InventorySlotWidget->SetSlotContainerType(EPDItemContainerType::None);

	if (ItemData.ItemID.IsNone())
	{
		InventorySlotWidget->ClearSlotData(EntryIndex);
		return;
	}

	InventorySlotWidget->SetSlotData(MakeMarketSlotData(ItemData), EntryIndex);
}

FPDInventorySlot UPDMarketItemWidget::MakeMarketSlotData(const FPDItemData& ItemData) const
{
	FPDInventorySlot SlotData;
	SlotData.ItemData = ItemData;
	SlotData.Quantity = Entry.Stock > 0 ? Entry.Stock : 1;
	SlotData.bIsEmpty = ItemData.ItemID.IsNone();
	return SlotData;
}

int32 UPDMarketItemWidget::GetUnitPrice() const
{
	return MarketComponent ? MarketComponent->GetEntryUnitPrice(Entry) : 0;
}

bool UPDMarketItemWidget::CanBuyCurrentEntry() const
{
	if (!MarketComponent || !BuyerInventory || EntryIndex == INDEX_NONE)
	{
		return false;
	}

	if (!MarketComponent->CanBuyEntry(EntryIndex))
	{
		return false;
	}

	if (Entry.Stock == 0)
	{
		return false;
	}

	return BuyerInventory->GetGold() >= GetUnitPrice();
}

FText UPDMarketItemWidget::MakePriceText(int32 Price) const
{
	return FText::Format(NSLOCTEXT("PDMarket", "MarketPriceFormat", "{0} Gold"), FText::AsNumber(FMath::Max(0, Price)));
}

void UPDMarketItemWidget::HandleBuyClicked()
{
	if (!MarketComponent || !BuyerInventory || EntryIndex == INDEX_NONE || !QuantityPopupWidgetClass)
	{
		return;
	}

	UPDMarketQuantityPopupWidget* PopupWidget = CreateWidget<UPDMarketQuantityPopupWidget>(GetOwningPlayer(), QuantityPopupWidgetClass);
	if (!PopupWidget)
	{
		return;
	}

	PopupWidget->InitializePurchasePopup(MarketComponent, BuyerInventory, Entry, EntryIndex);
	PopupWidget->AddToViewport(100);
}
