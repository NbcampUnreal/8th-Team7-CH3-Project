#include "Items/Market/PDMarketComponent.h"

#include "Core/PDGameInstance.h"
#include "Core/PDPlayerController.h"
#include "Core/PDPlayerState.h"
#include "Engine/DataTable.h"
#include "Net/UnrealNetwork.h"

namespace
{
	const FPDItemData* FindPDMarketItemDataByID(const UDataTable* DataTable, const FName& ItemID)
	{
		if (!DataTable || ItemID.IsNone())
		{
			return nullptr;
		}

		TArray<FPDItemData*> Rows;
		DataTable->GetAllRows<FPDItemData>(TEXT("FindPDMarketItemDataByID"), Rows);

		for (const FPDItemData* Row : Rows)
		{
			if (Row && Row->ItemID == ItemID)
			{
				return Row;
			}
		}

		return nullptr;
	}

	const TCHAR* DefaultMarketLevelDataTablePath = TEXT("/Game/Main/Blueprints/Data/DT_MarketLevelData.DT_MarketLevelData");
}

UPDMarketComponent::UPDMarketComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UPDMarketComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UPDMarketComponent, Goods);
	DOREPLIFETIME(UPDMarketComponent, TraderReputationExp);
	DOREPLIFETIME(UPDMarketComponent, TraderReputationLevel);
}

void UPDMarketComponent::OnRep_MarketData()
{
	OnTraderReputationChanged.Broadcast(TraderReputationLevel, TraderReputationExp);
	OnMarketChanged.Broadcast();
}

void UPDMarketComponent::BeginPlay()
{
	Super::BeginPlay();
	if (MarketGoodsDataTable)
	{
		ReloadGoodsFromDataTable();
	}
	SyncTraderReputationFromSave();
}

void UPDMarketComponent::ReloadGoodsFromDataTable()
{
	if (!MarketGoodsDataTable)
	{
		return;
	}

	Goods.Reset();

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
		Entry.ItemDataTable = ItemDataTable;
		Entry.ItemRowName = Row->ItemRowName.IsNone() ? RowName : Row->ItemRowName;
		Entry.Stock = Row->Stock;
		Entry.OverridePrice = Row->OverridePrice;

		FPDItemData ItemData;
		if (!ResolveEntryItemData(Entry, ItemData))
		{
			UE_LOG(LogTemp, Warning, TEXT("PDMarketComponent: Invalid market goods row '%s'. ItemRowName='%s'"),
				*RowName.ToString(), *Entry.ItemRowName.ToString());
			continue;
		}

		Goods.Add(Entry);
	}

	OnMarketChanged.Broadcast();
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

bool UPDMarketComponent::BuyEntry(UPDInventoryComponent* BuyerInventory, int32 EntryIndex, int32 Quantity)
{
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		if (const UWorld* World = GetWorld())
		{
			if (APDPlayerController* PC = Cast<APDPlayerController>(World->GetFirstPlayerController()))
			{
				PC->ServerBuyMarketEntry(this, EntryIndex, Quantity);
				return true;
			}
		}
		return false;
	}

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
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		if (const UWorld* World = GetWorld())
		{
			if (APDPlayerController* PC = Cast<APDPlayerController>(World->GetFirstPlayerController()))
			{
				PC->ServerSellInventorySlotToMarket(this, SlotIndex, Quantity);
				return true;
			}
		}
		return false;
	}

	if (!SellerInventory || Quantity <= 0 || !SellerInventory->Items.IsValidIndex(SlotIndex))
	{
		return false;
	}

	const FPDInventorySlot& SourceSlot = SellerInventory->Items[SlotIndex];
	if (SourceSlot.IsEmpty() || SourceSlot.ItemData.ItemID.IsNone())
	{
		return false;
	}

	const int32 SellQuantity = FMath::Min(Quantity, SourceSlot.Quantity);
	if (SellQuantity <= 0)
	{
		return false;
	}

	FPDMarketEntry* Entry = FindEntryByItemID(SourceSlot.ItemData.ItemID);
	if (SourceSlot.ItemData.bIsQuestItem)
	{
		return false;
	}

	const int32 UnitPrice = GetItemSellPrice(SourceSlot.ItemData);
	const int32 TotalPrice = UnitPrice * SellQuantity;

	FPDInventorySlot& MutableSourceSlot = SellerInventory->Items[SlotIndex];
	MutableSourceSlot.Quantity -= SellQuantity;
	if (MutableSourceSlot.Quantity <= 0)
	{
		MutableSourceSlot.Clear();
	}
	SellerInventory->OnInventoryChanged.Broadcast();

	SellerInventory->AddGold(TotalPrice);

	if (Entry && Entry->Stock >= 0)
	{
		Entry->Stock += SellQuantity;
	}

	OnMarketChanged.Broadcast();
	return true;
}

bool UPDMarketComponent::ResolveEntryItemData(const FPDMarketEntry& Entry, FPDItemData& OutItemData) const
{
	OutItemData = FPDItemData();

	if (!Entry.ItemDataTable || Entry.ItemRowName.IsNone())
	{
		return false;
	}

	const FPDItemData* Row = FindPDMarketItemDataByID(Entry.ItemDataTable, Entry.ItemRowName);
	if (!Row)
	{
		return false;
	}

	OutItemData = *Row;
	if (OutItemData.ItemID.IsNone())
	{
		OutItemData.ItemID = Entry.ItemRowName;
	}

	return true;
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
	return CanBuyEntryForInventory(EntryIndex, nullptr);
}

bool UPDMarketComponent::CanBuyEntryForInventory(int32 EntryIndex, const UPDInventoryComponent* BuyerInventory) const
{
	if (!Goods.IsValidIndex(EntryIndex))
	{
		return false;
	}

	FPDItemData ItemData;
	return ResolveEntryItemData(Goods[EntryIndex], ItemData) && CanBuyItemDataForInventory(ItemData, BuyerInventory);
}

bool UPDMarketComponent::CanBuyItemData(const FPDItemData& ItemData) const
{
	return CanBuyItemDataForInventory(ItemData, nullptr);
}

bool UPDMarketComponent::CanBuyItemDataForInventory(const FPDItemData& ItemData, const UPDInventoryComponent* BuyerInventory) const
{
	return true;
}

bool UPDMarketComponent::ShouldShowEntry(int32 EntryIndex) const
{
	return ShouldShowEntryForInventory(EntryIndex, nullptr);
}

bool UPDMarketComponent::ShouldShowEntryForInventory(int32 EntryIndex, const UPDInventoryComponent* BuyerInventory) const
{
	return Goods.IsValidIndex(EntryIndex);
}

int32 UPDMarketComponent::GetRequiredTraderLevelForGrade(EPDItemGrade ItemGrade) const
{
	return 1;
}

int32 UPDMarketComponent::GetRequiredTraderLevelForEntry(int32 EntryIndex) const
{
	return Goods.IsValidIndex(EntryIndex) ? 1 : INDEX_NONE;
}

EPDItemGrade UPDMarketComponent::GetMaxPurchasableGradeForLevel(int32 Level) const
{
	return EPDItemGrade::Grade5;
}


int32 UPDMarketComponent::GetCurrentTraderLevelRequiredExp() const
{
	return 0;
}

int32 UPDMarketComponent::GetNextTraderLevelRequiredExp() const
{
	return INDEX_NONE;
}

int32 UPDMarketComponent::GetCurrentTraderLevelDisplayExp() const
{
	return 0;
}

int32 UPDMarketComponent::GetCurrentTraderLevelDisplayExpForInventory(const UPDInventoryComponent* BuyerInventory) const
{
	return 0;
}

int32 UPDMarketComponent::GetNextTraderLevelDisplayRequiredExp() const
{
	return INDEX_NONE;
}

int32 UPDMarketComponent::GetNextTraderLevelDisplayRequiredExpForInventory(const UPDInventoryComponent* BuyerInventory) const
{
	return INDEX_NONE;
}

void UPDMarketComponent::AddTraderReputationExp(int32 Amount)
{
	return;
}

void UPDMarketComponent::SyncTraderReputationFromSave()
{
	TraderReputationExp = 0;
	TraderReputationLevel = 1;
}

void UPDMarketComponent::RecalculateTraderReputationLevel()
{
	TraderReputationLevel = 1;
}

const UDataTable* UPDMarketComponent::GetResolvedMarketLevelDataTable() const
{
	return nullptr;
}

const FPDMarketLevelData* UPDMarketComponent::FindMarketLevelDataByLevel(int32 Level) const
{
	return nullptr;
}

APDPlayerState* UPDMarketComponent::GetPlayerStateFromInventory(const UPDInventoryComponent* InventoryComponent) const
{
	return InventoryComponent ? Cast<APDPlayerState>(InventoryComponent->GetOwner()) : nullptr;
}

int32 UPDMarketComponent::GetTraderReputationExpForInventory(const UPDInventoryComponent* InventoryComponent) const
{
	return 0;
}

int32 UPDMarketComponent::GetTraderReputationLevelForInventory(const UPDInventoryComponent* InventoryComponent) const
{
	return 1;
}

int32 UPDMarketComponent::CalculateTraderReputationLevelFromExp(int32 ReputationExp) const
{
	return 1;
}

void UPDMarketComponent::AddTraderReputationExpForInventory(UPDInventoryComponent* InventoryComponent, int32 Amount)
{
	return;
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

void UPDMarketComponent::LoadTraderReputationFromSave()
{
	TraderReputationExp = 0;
	TraderReputationLevel = 1;
}

void UPDMarketComponent::SaveTraderReputationToSave() const
{
	return;
}

int32 UPDMarketComponent::CalculateReputationReward(int32 TotalPrice, int32 Percent) const
{
	return 0;
}
