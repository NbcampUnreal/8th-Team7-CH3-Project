#include "Items/PDMarketComponent.h"

#include "Engine/DataTable.h"

UPDMarketComponent::UPDMarketComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPDMarketComponent::BeginPlay()
{
	Super::BeginPlay();
	ReloadGoodsFromDataTable();
}

void UPDMarketComponent::ReloadGoodsFromDataTable()
{
	Goods.Reset();

	if (!MarketGoodsDataTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("PDMarketComponent: MarketGoodsDataTable is not set."));
		OnMarketChanged.Broadcast();
		return;
	}

	TArray<FName> RowNames = MarketGoodsDataTable->GetRowNames();
	Goods.Reserve(RowNames.Num());

	for (const FName& RowName : RowNames)
	{
		const FPDMarketGoodsRow* Row = MarketGoodsDataTable->FindRow<FPDMarketGoodsRow>(RowName, TEXT("ReloadGoodsFromDataTable"), false);
		if (!Row)
		{
			continue;
		}

		FPDMarketEntry Entry;
		Entry.ItemRowName = Row->ItemRowName.IsNone() ? RowName : Row->ItemRowName;
		Entry.Stock = Row->Stock;
		Entry.OverridePrice = Row->OverridePrice;

		FPDItemData ItemData;
		if (!ResolveEntryItemData(Entry, ItemData))
		{
			UE_LOG(LogTemp, Warning, TEXT("PDMarketComponent: Invalid market goods row '%s'. ItemRowName='%s'"), *RowName.ToString(), *Entry.ItemRowName.ToString());
			continue;
		}

		Goods.Add(Entry);
	}

	OnMarketChanged.Broadcast();
}

bool UPDMarketComponent::BuyEntry(UPDInventoryComponent* BuyerInventory, int32 EntryIndex, int32 Quantity)
{
	if (!BuyerInventory || !Goods.IsValidIndex(EntryIndex) || Quantity <= 0)
	{
		return false;
	}

	FPDMarketEntry& Entry = Goods[EntryIndex];
	FPDItemData ItemData;
	if (!ResolveEntryItemData(Entry, ItemData) || ItemData.ItemID.IsNone())
	{
		return false;
	}

	if (Entry.Stock >= 0 && Entry.Stock < Quantity)
	{
		return false;
	}

	const int32 UnitPrice = GetEntryUnitPrice(Entry);
	const int32 TotalPrice = UnitPrice * Quantity;

	if (!BuyerInventory->SpendGold(TotalPrice))
	{
		return false;
	}

	const int32 AddedQuantity = BuyerInventory->AddItemPartial(ItemData, Quantity);
	if (AddedQuantity != Quantity)
	{
		if (AddedQuantity > 0)
		{
			BuyerInventory->RemoveItem(ItemData.ItemID, AddedQuantity);
		}

		BuyerInventory->AddGold(TotalPrice);
		return false;
	}

	if (Entry.Stock >= 0)
	{
		Entry.Stock -= AddedQuantity;
	}

	OnMarketChanged.Broadcast();
	return true;
}

bool UPDMarketComponent::SellInventorySlot(UPDInventoryComponent* SellerInventory, int32 SlotIndex, int32 Quantity)
{
	if (!SellerInventory || Quantity <= 0 || !SellerInventory->Items.IsValidIndex(SlotIndex))
	{
		return false;
	}

	const FPDInventorySlot& SourceSlot = SellerInventory->Items[SlotIndex];
	if (SourceSlot.IsEmpty() || SourceSlot.ItemData.ItemID.IsNone() || SourceSlot.ItemData.bIsQuestItem)
	{
		return false;
	}

	const int32 SellQuantity = FMath::Min(Quantity, SourceSlot.Quantity);
	if (SellQuantity <= 0)
	{
		return false;
	}

	const int32 TotalPrice = GetItemSellPrice(SourceSlot.ItemData) * SellQuantity;

	FPDInventorySlot& MutableSourceSlot = SellerInventory->Items[SlotIndex];
	MutableSourceSlot.Quantity -= SellQuantity;
	if (MutableSourceSlot.Quantity <= 0)
	{
		MutableSourceSlot.Clear();
	}
	SellerInventory->OnInventoryChanged.Broadcast();

	SellerInventory->AddGold(TotalPrice);

	if (FPDMarketEntry* Entry = FindEntryByItemID(SourceSlot.ItemData.ItemID); Entry && Entry->Stock >= 0)
	{
		Entry->Stock += SellQuantity;
	}

	OnMarketChanged.Broadcast();
	return true;
}

bool UPDMarketComponent::ResolveEntryItemData(const FPDMarketEntry& Entry, FPDItemData& OutItemData) const
{
	OutItemData = FPDItemData();

	const FPDItemData* Row = FindItemData(Entry.ItemRowName);
	if (!Row)
	{
		return false;
	}

	OutItemData = *Row;
	if (OutItemData.ItemID.IsNone())
	{
		OutItemData.ItemID = Entry.ItemRowName;
	}

	return !OutItemData.ItemID.IsNone();
}

int32 UPDMarketComponent::GetEntryUnitPrice(const FPDMarketEntry& Entry) const
{
	if (Entry.OverridePrice >= 0)
	{
		return Entry.OverridePrice;
	}

	FPDItemData ItemData;
	return ResolveEntryItemData(Entry, ItemData) ? GetItemBuyPrice(ItemData) : 0;
}

int32 UPDMarketComponent::GetItemBuyPrice(const FPDItemData& ItemData) const
{
	return FMath::Max(0, ItemData.Price);
}

int32 UPDMarketComponent::GetItemSellPrice(const FPDItemData& ItemData) const
{
	const int32 BuyPrice = GetItemBuyPrice(ItemData);
	if (BuyPrice <= 0 || SellPriceRate <= 0.f)
	{
		return 0;
	}

	return FMath::Max(1, FMath::FloorToInt(static_cast<float>(BuyPrice) * SellPriceRate));
}

bool UPDMarketComponent::CanBuyEntry(int32 EntryIndex) const
{
	if (!Goods.IsValidIndex(EntryIndex))
	{
		return false;
	}

	FPDItemData ItemData;
	return ResolveEntryItemData(Goods[EntryIndex], ItemData);
}

bool UPDMarketComponent::CanBuyItemData(const FPDItemData& ItemData) const
{
	return !ItemData.ItemID.IsNone();
}

bool UPDMarketComponent::ShouldShowEntry(int32 EntryIndex) const
{
	FPDItemData ItemData;
	return Goods.IsValidIndex(EntryIndex) && ResolveEntryItemData(Goods[EntryIndex], ItemData);
}

FPDMarketEntry* UPDMarketComponent::FindEntryByItemID(FName ItemID)
{
	if (ItemID.IsNone())
	{
		return nullptr;
	}

	for (FPDMarketEntry& Entry : Goods)
	{
		FPDItemData EntryItemData;
		if (ResolveEntryItemData(Entry, EntryItemData) && EntryItemData.ItemID == ItemID)
		{
			return &Entry;
		}
	}

	return nullptr;
}

const FPDItemData* UPDMarketComponent::FindItemData(FName ItemRowName) const
{
	if (!ItemDataTable || ItemRowName.IsNone())
	{
		return nullptr;
	}

	if (const FPDItemData* DirectRow = ItemDataTable->FindRow<FPDItemData>(ItemRowName, TEXT("FindItemData"), false))
	{
		return DirectRow;
	}

	TArray<FPDItemData*> Rows;
	ItemDataTable->GetAllRows<FPDItemData>(TEXT("FindItemData"), Rows);
	for (const FPDItemData* Row : Rows)
	{
		if (Row && Row->ItemID == ItemRowName)
		{
			return Row;
		}
	}

	return nullptr;
}
