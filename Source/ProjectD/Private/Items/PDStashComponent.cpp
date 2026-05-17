#include "Items/PDStashComponent.h"

#include "Core/PDGameInstance.h"
#include "Items/PDItemSlotTransfer.h"
#include "Items/PDItemSoundLibrary.h"

UPDStashComponent::UPDStashComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	GridColumns = 5;
	BaseGridRows = 4;
	GridRows = BaseGridRows;
	CurrentUpgradeLevel = 0;

	UpgradeData.SetNum(3);
	UpgradeData[0].Cost = 1000;
	UpgradeData[0].AddedRows = 1;
	UpgradeData[1].Cost = 2500;
	UpgradeData[1].AddedRows = 1;
	UpgradeData[2].Cost = 5000;
	UpgradeData[2].AddedRows = 1;
}

void UPDStashComponent::BeginPlay()
{
	Super::BeginPlay();

	LoadFromGameInstance();
}

void UPDStashComponent::LoadFromGameInstance()
{
	const UPDGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance<UPDGameInstance>() : nullptr;
	if (GI)
	{
		StashItems = GI->GetStashItems();
		SetStashUpgradeLevel(GI->GetStashUpgradeLevel());
	}
	else
	{
		SetStashUpgradeLevel(CurrentUpgradeLevel);
	}

	InitializeStash();
}

void UPDStashComponent::SaveToGameInstance()
{
	UPDGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance<UPDGameInstance>() : nullptr;
	if (!GI)
	{
		return;
	}

	GI->SetStashItems(StashItems);
	GI->SetStashUpgradeLevel(CurrentUpgradeLevel);
}


int32 UPDStashComponent::GetNextUpgradeCost() const
{
	return UpgradeData.IsValidIndex(CurrentUpgradeLevel) ? FMath::Max(0, UpgradeData[CurrentUpgradeLevel].Cost) : 0;
}

int32 UPDStashComponent::GetNextUpgradeAddedRows() const
{
	return UpgradeData.IsValidIndex(CurrentUpgradeLevel) ? FMath::Max(0, UpgradeData[CurrentUpgradeLevel].AddedRows) : 0;
}

EPDStashUpgradeResult UPDStashComponent::CanUpgradeStash(const UPDInventoryComponent* SourceInventory) const
{
	if (!SourceInventory)
	{
		return EPDStashUpgradeResult::InvalidInventory;
	}

	if (IsMaxUpgradeLevel())
	{
		return EPDStashUpgradeResult::AlreadyMaxLevel;
	}

	const int32 UpgradeCost = GetNextUpgradeCost();
	const int32 AddedRows = GetNextUpgradeAddedRows();
	if (UpgradeCost <= 0 || AddedRows <= 0)
	{
		return EPDStashUpgradeResult::InvalidConfig;
	}

	if (SourceInventory->GetGold() < UpgradeCost)
	{
		return EPDStashUpgradeResult::NotEnoughGold;
	}

	return EPDStashUpgradeResult::Success;
}

EPDStashUpgradeResult UPDStashComponent::UpgradeStash(UPDInventoryComponent* SourceInventory)
{
	const EPDStashUpgradeResult Result = CanUpgradeStash(SourceInventory);
	if (Result != EPDStashUpgradeResult::Success)
	{
		OnStashUpgradeFailed.Broadcast(Result);
		return Result;
	}

	const int32 UpgradeCost = GetNextUpgradeCost();
	const int32 AddedRows = GetNextUpgradeAddedRows();

	if (!SourceInventory->SpendGold(UpgradeCost))
	{
		OnStashUpgradeFailed.Broadcast(EPDStashUpgradeResult::NotEnoughGold);
		return EPDStashUpgradeResult::NotEnoughGold;
	}

	++CurrentUpgradeLevel;
	GridRows += AddedRows;

	InitializeStash();
	SaveToGameInstance();
	OnStashUpgraded.Broadcast(CurrentUpgradeLevel, GridRows);
	return EPDStashUpgradeResult::Success;
}

void UPDStashComponent::SetStashUpgradeLevel(int32 NewUpgradeLevel)
{
	CurrentUpgradeLevel = FMath::Clamp(NewUpgradeLevel, 0, GetMaxUpgradeLevel());

	GridRows = FMath::Max(1, BaseGridRows);
	for (int32 Index = 0; Index < CurrentUpgradeLevel && UpgradeData.IsValidIndex(Index); ++Index)
	{
		GridRows += FMath::Max(0, UpgradeData[Index].AddedRows);
	}
}

int32 UPDStashComponent::FindEmptySlot() const
{
	for (int32 Index = 0; Index < StashItems.Num(); ++Index)
	{
		if (StashItems[Index].IsEmpty())
		{
			return Index;
		}
	}

	return INDEX_NONE;
}

void UPDStashComponent::InitializeStash()
{
	const int32 MaxSlotCount = GetMaxSlotCount();

	if (MaxSlotCount <= 0)
	{
		StashItems.Empty();
		OnStashChanged.Broadcast();
		return;
	}

	const int32 OldCount = StashItems.Num();

	StashItems.SetNum(MaxSlotCount);

	for (int32 Index = OldCount; Index < StashItems.Num(); ++Index)
	{
		StashItems[Index].Clear();
	}

	for (FPDInventorySlot& Slot : StashItems)
	{
		if (Slot.IsEmpty())
		{
			Slot.Clear();
		}
	}

	OnStashChanged.Broadcast();
}

void UPDStashComponent::ResetStash()
{
	StashItems.SetNum(GetMaxSlotCount());

	for (FPDInventorySlot& Slot : StashItems)
	{
		Slot.Clear();
	}

	SaveToGameInstance();
	OnStashChanged.Broadcast();
}

int32 UPDStashComponent::AddItemPartial(const FPDItemData& ItemData, int32 Quantity)
{
	if (ItemData.ItemID.IsNone() || Quantity <= 0)
	{
		return 0;
	}

	if (StashItems.Num() != GetMaxSlotCount())
	{
		InitializeStash();
	}

	int32 RemainingQuantity = Quantity;
	int32 AddedQuantity = 0;

	const int32 MaxStack = FMath::Max(1, ItemData.MaxStack);

	if (MaxStack > 1)
	{
		for (FPDInventorySlot& Slot : StashItems)
		{
			if (!Slot.IsEmpty() && Slot.ItemData.ItemID == ItemData.ItemID && Slot.Quantity < MaxStack)
			{
				const int32 AddAmount = FMath::Min(RemainingQuantity, MaxStack - Slot.Quantity);
				Slot.Quantity += AddAmount;
				RemainingQuantity -= AddAmount;
				AddedQuantity += AddAmount;

				if (RemainingQuantity <= 0)
				{
					UPDItemSoundLibrary::PlayItemMoveSound(this, ItemData);
					SaveToGameInstance();
					OnStashChanged.Broadcast();
					return AddedQuantity;
				}
			}
		}
	}

	while (RemainingQuantity > 0)
	{
		const int32 EmptySlot = FindEmptySlot();

		if (EmptySlot == INDEX_NONE)
		{
			break;
		}

		const int32 AddAmount = FMath::Min(RemainingQuantity, MaxStack);

		StashItems[EmptySlot].ItemData = ItemData;
		StashItems[EmptySlot].Quantity = AddAmount;
		StashItems[EmptySlot].bIsEmpty = false;

		RemainingQuantity -= AddAmount;
		AddedQuantity += AddAmount;
	}

	if (AddedQuantity > 0)
	{
		UPDItemSoundLibrary::PlayItemMoveSound(this, ItemData);
		SaveToGameInstance();
		OnStashChanged.Broadcast();
	}

	return AddedQuantity;
}


bool UPDStashComponent::AddItemByID(FName ItemID, int32 Quantity)
{
	if (!ItemDataTable || ItemID.IsNone()) return false;

	TArray<FPDItemData*> Rows;
	ItemDataTable->GetAllRows<FPDItemData>(TEXT("AddItemByID"), Rows);

	for (const FPDItemData* Row : Rows)
	{
		if (Row && Row->ItemID == ItemID)
			return AddItemPartial(*Row, Quantity) > 0;
	}

	return false;
}

int32 UPDStashComponent::AddItemToSlotPartial(const FPDItemData& ItemData, int32 Quantity, int32 TargetSlotIndex)
{
	if (StashItems.Num() != GetMaxSlotCount())
	{
		InitializeStash();
	}

	if (!StashItems.IsValidIndex(TargetSlotIndex))
	{
		return 0;
	}

	const int32 AddedQuantity = FPDItemSlotTransfer::AddItemToSlot(StashItems[TargetSlotIndex], ItemData, Quantity);
	if (AddedQuantity > 0)
	{
		UPDItemSoundLibrary::PlayItemMoveSound(this, ItemData);
		SaveToGameInstance();
		OnStashChanged.Broadcast();
	}

	return AddedQuantity;
}

bool UPDStashComponent::MoveSlotQuantityToSlot(int32 SourceSlotIndex, int32 TargetSlotIndex, int32 Quantity)
{
	if (StashItems.Num() != GetMaxSlotCount())
	{
		InitializeStash();
	}

	if (!StashItems.IsValidIndex(SourceSlotIndex) || !StashItems.IsValidIndex(TargetSlotIndex) || SourceSlotIndex == TargetSlotIndex || Quantity <= 0)
	{
		return false;
	}

	const FPDItemData MovedItemData = StashItems[SourceSlotIndex].ItemData;
	const bool bMoved = FPDItemSlotTransfer::MoveQuantity(StashItems[SourceSlotIndex], StashItems[TargetSlotIndex], Quantity);
	if (bMoved)
	{
		UPDItemSoundLibrary::PlayItemMoveSound(this, MovedItemData);
		SaveToGameInstance();
		OnStashChanged.Broadcast();
	}

	return bMoved;
}

bool UPDStashComponent::RemoveItem(FName ItemID, int32 Quantity)
{
	if (ItemID.IsNone() || Quantity <= 0 || !HasItem(ItemID, Quantity))
	{
		return false;
	}

	for (int32 Index = StashItems.Num() - 1; Index >= 0 && Quantity > 0; --Index)
	{
		FPDInventorySlot& Slot = StashItems[Index];

		if (Slot.IsEmpty() || Slot.ItemData.ItemID != ItemID)
		{
			continue;
		}

		const int32 RemoveAmount = FMath::Min(Quantity, Slot.Quantity);
		Slot.Quantity -= RemoveAmount;
		Quantity -= RemoveAmount;

		if (Slot.Quantity <= 0)
		{
			Slot.Clear();
		}
	}

	SaveToGameInstance();
	OnStashChanged.Broadcast();
	return true;
}

bool UPDStashComponent::StoreInventorySlot(UPDInventoryComponent* SourceInventory, int32 SourceSlotIndex)
{
	if (!SourceInventory || !SourceInventory->Items.IsValidIndex(SourceSlotIndex))
	{
		return false;
	}

	const FPDInventorySlot& SourceSlot = SourceInventory->Items[SourceSlotIndex];
	return StoreInventorySlotQuantity(SourceInventory, SourceSlotIndex, SourceSlot.IsEmpty() ? 0 : SourceSlot.Quantity);
}

bool UPDStashComponent::StoreInventorySlotQuantity(UPDInventoryComponent* SourceInventory, int32 SourceSlotIndex, int32 Quantity)
{
	if (!SourceInventory || !SourceInventory->Items.IsValidIndex(SourceSlotIndex) || Quantity <= 0)
	{
		return false;
	}

	FPDInventorySlot& SourceSlot = SourceInventory->Items[SourceSlotIndex];

	if (SourceSlot.IsEmpty())
	{
		return false;
	}

	const int32 MoveQuantity = FMath::Min(Quantity, SourceSlot.Quantity);
	const int32 AddedQuantity = AddItemPartial(SourceSlot.ItemData, MoveQuantity);

	if (AddedQuantity <= 0)
	{
		return false;
	}

	SourceSlot.Quantity -= AddedQuantity;

	if (SourceSlot.Quantity <= 0)
	{
		SourceSlot.Clear();
	}

	SourceInventory->OnInventoryChanged.Broadcast();
	return true;
}


bool UPDStashComponent::StoreInventorySlotQuantityToSlot(UPDInventoryComponent* SourceInventory, int32 SourceSlotIndex, int32 TargetStashSlotIndex, int32 Quantity)
{
	if (!SourceInventory || SourceInventory->Items.Num() != SourceInventory->GetMaxSlotCount())
	{
		if (SourceInventory)
		{
			SourceInventory->InitializeInventory();
		}
	}

	if (StashItems.Num() != GetMaxSlotCount())
	{
		InitializeStash();
	}

	if (!SourceInventory || !SourceInventory->Items.IsValidIndex(SourceSlotIndex) || !StashItems.IsValidIndex(TargetStashSlotIndex) || Quantity <= 0)
	{
		return false;
	}

	const FPDItemData MovedItemData = SourceInventory->Items[SourceSlotIndex].ItemData;
	const bool bMoved = FPDItemSlotTransfer::MoveQuantity(SourceInventory->Items[SourceSlotIndex], StashItems[TargetStashSlotIndex], Quantity);
	if (bMoved)
	{
		UPDItemSoundLibrary::PlayItemMoveSound(this, MovedItemData);
		SourceInventory->OnInventoryChanged.Broadcast();
		SaveToGameInstance();
		OnStashChanged.Broadcast();
	}

	return bMoved;
}

bool UPDStashComponent::TakeStashSlot(UPDInventoryComponent* TargetInventory, int32 StashSlotIndex)
{
	if (!StashItems.IsValidIndex(StashSlotIndex))
	{
		return false;
	}

	const FPDInventorySlot& SourceSlot = StashItems[StashSlotIndex];
	return TakeStashSlotQuantity(TargetInventory, StashSlotIndex, SourceSlot.IsEmpty() ? 0 : SourceSlot.Quantity);
}

bool UPDStashComponent::TakeStashSlotQuantity(UPDInventoryComponent* TargetInventory, int32 StashSlotIndex, int32 Quantity)
{
	if (!TargetInventory || !StashItems.IsValidIndex(StashSlotIndex) || Quantity <= 0)
	{
		return false;
	}

	FPDInventorySlot& SourceSlot = StashItems[StashSlotIndex];

	if (SourceSlot.IsEmpty())
	{
		return false;
	}

	const int32 MoveQuantity = FMath::Min(Quantity, SourceSlot.Quantity);
	const int32 AddedQuantity = TargetInventory->AddItemPartial(SourceSlot.ItemData, MoveQuantity);

	if (AddedQuantity <= 0)
	{
		return false;
	}

	SourceSlot.Quantity -= AddedQuantity;

	if (SourceSlot.Quantity <= 0)
	{
		SourceSlot.Clear();
	}

	SaveToGameInstance();
	OnStashChanged.Broadcast();
	return true;
}


bool UPDStashComponent::TakeStashSlotQuantityToInventorySlot(UPDInventoryComponent* TargetInventory, int32 StashSlotIndex, int32 TargetInventorySlotIndex, int32 Quantity)
{
	if (!TargetInventory || TargetInventory->Items.Num() != TargetInventory->GetMaxSlotCount())
	{
		if (TargetInventory)
		{
			TargetInventory->InitializeInventory();
		}
	}

	if (StashItems.Num() != GetMaxSlotCount())
	{
		InitializeStash();
	}

	if (!TargetInventory || !StashItems.IsValidIndex(StashSlotIndex) || !TargetInventory->Items.IsValidIndex(TargetInventorySlotIndex) || Quantity <= 0)
	{
		return false;
	}

	const FPDInventorySlot& SourceSlot = StashItems[StashSlotIndex];
	if (!TargetInventory->CanAddSlotWeight(SourceSlot, Quantity))
	{
		TargetInventory->BroadcastWeightLimitExceeded();
		return false;
	}

	const FPDItemData MovedItemData = StashItems[StashSlotIndex].ItemData;
	const bool bMoved = FPDItemSlotTransfer::MoveQuantity(StashItems[StashSlotIndex], TargetInventory->Items[TargetInventorySlotIndex], Quantity);
	if (bMoved)
	{
		UPDItemSoundLibrary::PlayItemMoveSound(this, MovedItemData);
		SaveToGameInstance();
		OnStashChanged.Broadcast();
		TargetInventory->OnInventoryChanged.Broadcast();
	}

	return bMoved;
}

bool UPDStashComponent::HasItem(FName ItemID, int32 Quantity) const
{
	if (ItemID.IsNone() || Quantity <= 0)
	{
		return false;
	}

	int32 FoundQuantity = 0;

	for (const FPDInventorySlot& Slot : StashItems)
	{
		if (Slot.IsEmpty())
		{
			continue;
		}

		if (Slot.ItemData.ItemID == ItemID)
		{
			FoundQuantity += Slot.Quantity;

			if (FoundQuantity >= Quantity)
			{
				return true;
			}
		}
	}

	return false;
}
