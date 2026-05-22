#include "Widgets/Inventory/PDMarketQuantityPopupWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "InputCoreTypes.h"
#include "Items/PDInventoryComponent.h"
#include "Widgets/Inventory/PDInventorySlotWidget.h"

void UPDMarketQuantityPopupWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	SetIsFocusable(true);
	ResolveWidgets();
}

void UPDMarketQuantityPopupWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetKeyboardFocus();
	ResolveWidgets();
	RefreshVisuals();
}

FReply UPDMarketQuantityPopupWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		HandleCancelClicked();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UPDMarketQuantityPopupWidget::InitializePurchasePopup(UPDMarketComponent* InMarketComponent, UPDInventoryComponent* InBuyerInventory, const FPDMarketEntry& InEntry, int32 InEntryIndex)
{
	MarketComponent = InMarketComponent;
	BuyerInventory = InBuyerInventory;
	Entry = InEntry;
	EntryIndex = InEntryIndex;
	ItemData = FPDItemData();
	Quantity = 1;

	if (MarketComponent)
	{
		MarketComponent->ResolveEntryItemData(Entry, ItemData);
	}

	SetQuantity(1);
	ResolveWidgets();
	RefreshVisuals();
}

void UPDMarketQuantityPopupWidget::ResolveWidgets()
{
	if (!WidgetTree)
	{
		return;
	}

	if (!TextTitleWidget)
	{
		TextTitleWidget = Cast<UTextBlock>(WidgetTree->FindWidget(TextTitleWidgetName));
	}

	if (!InventorySlotWidget)
	{
		InventorySlotWidget = Cast<UPDInventorySlotWidget>(WidgetTree->FindWidget(InventorySlotWidgetName));
	}

	if (!InventorySlotWidget)
	{
		WidgetTree->ForEachWidget([this](UWidget* Widget)
		{
			if (!InventorySlotWidget)
			{
				InventorySlotWidget = Cast<UPDInventorySlotWidget>(Widget);
			}
		});
	}

	if (!ImageItemIconWidget)
	{
		ImageItemIconWidget = Cast<UImage>(WidgetTree->FindWidget(ImageItemIconWidgetName));
	}

	if (!TextItemNameWidget)
	{
		TextItemNameWidget = Cast<UTextBlock>(WidgetTree->FindWidget(TextItemNameWidgetName));
	}

	if (!TextItemDescWidget)
	{
		TextItemDescWidget = Cast<UTextBlock>(WidgetTree->FindWidget(TextItemDescWidgetName));
	}

	if (!TextItemDescWidget)
	{
		TextItemDescWidget = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("Text_Description")));
	}

	if (!TextPriceWidget)
	{
		TextPriceWidget = Cast<UTextBlock>(WidgetTree->FindWidget(TextPriceWidgetName));
	}

	if (!TextGoldWidget)
	{
		TextGoldWidget = Cast<UTextBlock>(WidgetTree->FindWidget(TextGoldWidgetName));
	}

	if (!TextQuantityWidget)
	{
		TextQuantityWidget = Cast<UTextBlock>(WidgetTree->FindWidget(TextQuantityWidgetName));
	}

	if (!TextHaveCountWidget)
	{
		TextHaveCountWidget = Cast<UTextBlock>(WidgetTree->FindWidget(TextHaveCountWidgetName));
	}

	if (!TextTotalPriceWidget)
	{
		TextTotalPriceWidget = Cast<UTextBlock>(WidgetTree->FindWidget(TextTotalPriceWidgetName));
	}

	if (!TextTotalPriceWidget)
	{
		TextTotalPriceWidget = Cast<UTextBlock>(WidgetTree->FindWidget(TextBuyPriceWidgetName));
	}

	ResolveButton(ButtonCloseWidget, ButtonCloseWidgetName);
	ResolveButton(ButtonMinusWidget, ButtonMinusWidgetName);
	ResolveButton(ButtonPlusWidget, ButtonPlusWidgetName);
	ResolveButton(ButtonMaxWidget, ButtonMaxWidgetName);
	ResolveButton(ButtonCancelWidget, ButtonCancelWidgetName);
	ResolveButton(ButtonBuyWidget, ButtonBuyWidgetName);

	if (ButtonCloseWidget)
	{
		ButtonCloseWidget->OnClicked.RemoveDynamic(this, &UPDMarketQuantityPopupWidget::HandleCancelClicked);
		ButtonCloseWidget->OnClicked.AddUniqueDynamic(this, &UPDMarketQuantityPopupWidget::HandleCancelClicked);
	}

	if (ButtonMinusWidget)
	{
		ButtonMinusWidget->OnClicked.RemoveDynamic(this, &UPDMarketQuantityPopupWidget::HandleMinusClicked);
		ButtonMinusWidget->OnClicked.AddUniqueDynamic(this, &UPDMarketQuantityPopupWidget::HandleMinusClicked);
	}

	if (ButtonPlusWidget)
	{
		ButtonPlusWidget->OnClicked.RemoveDynamic(this, &UPDMarketQuantityPopupWidget::HandlePlusClicked);
		ButtonPlusWidget->OnClicked.AddUniqueDynamic(this, &UPDMarketQuantityPopupWidget::HandlePlusClicked);
	}

	if (ButtonMaxWidget)
	{
		ButtonMaxWidget->OnClicked.RemoveDynamic(this, &UPDMarketQuantityPopupWidget::HandleMaxClicked);
		ButtonMaxWidget->OnClicked.AddUniqueDynamic(this, &UPDMarketQuantityPopupWidget::HandleMaxClicked);
	}

	if (ButtonCancelWidget)
	{
		ButtonCancelWidget->OnClicked.RemoveDynamic(this, &UPDMarketQuantityPopupWidget::HandleCancelClicked);
		ButtonCancelWidget->OnClicked.AddUniqueDynamic(this, &UPDMarketQuantityPopupWidget::HandleCancelClicked);
	}

	if (ButtonBuyWidget)
	{
		ButtonBuyWidget->OnClicked.RemoveDynamic(this, &UPDMarketQuantityPopupWidget::HandleBuyClicked);
		ButtonBuyWidget->OnClicked.AddUniqueDynamic(this, &UPDMarketQuantityPopupWidget::HandleBuyClicked);
	}
}

void UPDMarketQuantityPopupWidget::ResolveButton(TObjectPtr<UButton>& Button, FName WidgetName)
{
	if (!Button && !WidgetName.IsNone() && WidgetTree)
	{
		Button = Cast<UButton>(WidgetTree->FindWidget(WidgetName));
	}
}

void UPDMarketQuantityPopupWidget::RefreshVisuals()
{
	ResolveWidgets();

	if (TextTitleWidget)
	{
		TextTitleWidget->SetText(NSLOCTEXT("PDMarket", "BuyPopupTitle", "구매하기"));
	}

	if (TextItemNameWidget)
	{
		TextItemNameWidget->SetText(!ItemData.DisplayName.IsEmpty() ? ItemData.DisplayName : FText::FromName(Entry.ItemRowName));
	}

	if (TextItemDescWidget)
	{
		TextItemDescWidget->SetText(ItemData.Description);
	}

	if (TextPriceWidget)
	{
		TextPriceWidget->SetText(MakeGoldText(GetUnitPrice()));
	}

	if (TextGoldWidget)
	{
		TextGoldWidget->SetText(MakeGoldText(BuyerInventory ? BuyerInventory->GetGold() : 0));
	}

	if (ImageItemIconWidget)
	{
		ImageItemIconWidget->SetBrushFromTexture(ItemData.Icon, true);
	}

	if (InventorySlotWidget)
	{
		FPDInventorySlot SlotData;
		SlotData.ItemData = ItemData;
		SlotData.Quantity = 1;
		SlotData.bIsEmpty = ItemData.ItemID.IsNone();
		InventorySlotWidget->SetSlotContainerType(EPDItemContainerType::None);
		InventorySlotWidget->SetSlotData(SlotData, EntryIndex);
	}

	RefreshBuyState();
}

void UPDMarketQuantityPopupWidget::RefreshBuyState()
{
	const int32 MaxQuantity = GetMaxBuyQuantity();
	Quantity = FMath::Clamp(Quantity, 1, FMath::Max(1, MaxQuantity));

	if (TextQuantityWidget)
	{
		TextQuantityWidget->SetText(FText::AsNumber(Quantity));
	}

	if (TextHaveCountWidget)
	{
		TextHaveCountWidget->SetText(FText::Format(NSLOCTEXT("PDMarket", "HaveCountFormat", "보유: {0}개"), FText::AsNumber(GetCurrentOwnedCount())));
	}

	if (TextTotalPriceWidget)
	{
		TextTotalPriceWidget->SetText(MakeGoldText(GetTotalPrice()));
	}

	if (ButtonMinusWidget)
	{
		ButtonMinusWidget->SetIsEnabled(Quantity > 1);
	}

	if (ButtonPlusWidget)
	{
		ButtonPlusWidget->SetIsEnabled(Quantity < MaxQuantity);
	}

	if (ButtonMaxWidget)
	{
		ButtonMaxWidget->SetIsEnabled(MaxQuantity > 1);
	}

	if (ButtonBuyWidget)
	{
		ButtonBuyWidget->SetIsEnabled(CanBuy());
	}
}

void UPDMarketQuantityPopupWidget::SetQuantity(int32 NewQuantity)
{
	Quantity = FMath::Clamp(NewQuantity, 1, FMath::Max(1, GetMaxBuyQuantity()));
	RefreshBuyState();
}

int32 UPDMarketQuantityPopupWidget::GetMaxBuyQuantity() const
{
	if (!BuyerInventory || !MarketComponent)
	{
		return 0;
	}

	const int32 UnitPrice = GetUnitPrice();
	if (UnitPrice <= 0)
	{
		return 0;
	}

	int32 MaxQuantity = BuyerInventory->GetGold() / UnitPrice;
	if (Entry.Stock >= 0)
	{
		MaxQuantity = FMath::Min(MaxQuantity, Entry.Stock);
	}

	return FMath::Max(0, MaxQuantity);
}

int32 UPDMarketQuantityPopupWidget::GetCurrentOwnedCount() const
{
	if (!BuyerInventory || ItemData.ItemID.IsNone())
	{
		return 0;
	}

	int32 Count = 0;
	for (const FPDInventorySlot& InventorySlot : BuyerInventory->Items)
	{
		if (!InventorySlot.IsEmpty() && InventorySlot.ItemData.ItemID == ItemData.ItemID)
		{
			Count += InventorySlot.Quantity;
		}
	}

	return Count;
}

int32 UPDMarketQuantityPopupWidget::GetUnitPrice() const
{
	return MarketComponent ? MarketComponent->GetEntryUnitPrice(Entry) : 0;
}

int32 UPDMarketQuantityPopupWidget::GetTotalPrice() const
{
	return GetUnitPrice() * Quantity;
}

bool UPDMarketQuantityPopupWidget::CanBuy() const
{
	return MarketComponent && BuyerInventory && EntryIndex != INDEX_NONE && Quantity > 0 && Quantity <= GetMaxBuyQuantity();
}

FText UPDMarketQuantityPopupWidget::MakeGoldText(int32 Gold) const
{
	return FText::Format(NSLOCTEXT("PDMarket", "GoldFormat", "{0} Gold"), FText::AsNumber(FMath::Max(0, Gold)));
}

void UPDMarketQuantityPopupWidget::HandleMinusClicked()
{
	SetQuantity(Quantity - 1);
}

void UPDMarketQuantityPopupWidget::HandlePlusClicked()
{
	SetQuantity(Quantity + 1);
}

void UPDMarketQuantityPopupWidget::HandleMaxClicked()
{
	SetQuantity(GetMaxBuyQuantity());
}

void UPDMarketQuantityPopupWidget::HandleCancelClicked()
{
	RemoveFromParent();
}

void UPDMarketQuantityPopupWidget::HandleBuyClicked()
{
	if (!CanBuy())
	{
		RefreshBuyState();
		return;
	}

	if (MarketComponent->BuyEntry(BuyerInventory, EntryIndex, Quantity))
	{
		RemoveFromParent();
		return;
	}

	RefreshVisuals();
}
