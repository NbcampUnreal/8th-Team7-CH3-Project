#include "Items/PDInventoryComponent.h"

#include "Items/PDItemSlotTransfer.h"

UPDInventoryComponent::UPDInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPDInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	InitializeInventory();
}

bool UPDInventoryComponent::AddItem(const FPDItemData& ItemData, int32 Quantity)
{
	return AddItemPartial(ItemData, Quantity) > 0;
}

bool UPDInventoryComponent::RemoveItem(FName ItemID, int32 Quantity)
{
	if (ItemID.IsNone() || Quantity <= 0 || !HasItem(ItemID, Quantity))
	{
		return false;
	}

	for (int32 Index = Items.Num() - 1; Index >= 0 && Quantity > 0; --Index)
	{
		FPDInventorySlot& Slot = Items[Index];

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

	OnInventoryChanged.Broadcast();
	return true;
}

bool UPDInventoryComponent::RemoveItemFromSlot(int32 SlotIndex, int32 Quantity)
{
	if (!Items.IsValidIndex(SlotIndex) || Quantity <= 0)
	{
		return false;
	}

	FPDInventorySlot& Slot = Items[SlotIndex];
	if (Slot.IsEmpty())
	{
		return false;
	}

	const int32 RemoveAmount = FMath::Min(Quantity, Slot.Quantity);
	Slot.Quantity -= RemoveAmount;

	if (Slot.Quantity <= 0)
	{
		Slot.Clear();
	}

	OnInventoryChanged.Broadcast();
	return true;
}

bool UPDInventoryComponent::DropItemFromSlot(int32 SlotIndex, int32 Quantity)
{
	return RemoveItemFromSlot(SlotIndex, Quantity);
}

bool UPDInventoryComponent::UseItemFromSlot(int32 SlotIndex)
{
	if (!Items.IsValidIndex(SlotIndex))
	{
		return false;
	}

	const FPDInventorySlot& Slot = Items[SlotIndex];
	if (Slot.IsEmpty() || Slot.ItemData.ItemType != EPDItemType::Consumable)
	{
		return false;
	}

	// TODO: Apply the item's GameplayEffectClass here when consumable effects are added to FPDItemData.
	return RemoveItemFromSlot(SlotIndex, 1);
}

bool UPDInventoryComponent::HasItem(FName ItemID, int32 Quantity) const
{
	if (ItemID.IsNone() || Quantity <= 0)
	{
		return false;
	}

	int32 TotalQuantity = 0;

	for (const FPDInventorySlot& Slot : Items)
	{
		if (Slot.IsEmpty())
		{
			continue;
		}

		if (Slot.ItemData.ItemID == ItemID)
		{
			TotalQuantity += Slot.Quantity;

			if (TotalQuantity >= Quantity)
			{
				return true;
			}
		}
	}

	return false;
}

void UPDInventoryComponent::AddGold(int32 Amount)
{
	if (Amount > 0)
	{
		Gold += Amount;
		OnInventoryChanged.Broadcast();
	}
}

bool UPDInventoryComponent::SpendGold(int32 Amount)
{
	if (Amount <= 0 || Gold < Amount)
	{
		return false;
	}

	Gold -= Amount;
	OnInventoryChanged.Broadcast();
	return true;
}

int32 UPDInventoryComponent::FindEmptySlot() const
{
	for (int32 i = 0; i < Items.Num(); i++)
	{
		if (Items[i].IsEmpty())
		{
			return i;
		}
	}

	return INDEX_NONE;
}


int32 UPDInventoryComponent::AddItemToSlotPartial(const FPDItemData& ItemData, int32 Quantity, int32 TargetSlotIndex)
{
	if (Items.Num() != GetMaxSlotCount())
	{
		InitializeInventory();
	}

	if (!Items.IsValidIndex(TargetSlotIndex))
	{
		return 0;
	}

	const int32 AddedQuantity = FPDItemSlotTransfer::AddItemToSlot(Items[TargetSlotIndex], ItemData, Quantity);
	if (AddedQuantity > 0)
	{
		OnInventoryChanged.Broadcast();
	}

	return AddedQuantity;
}

bool UPDInventoryComponent::MoveSlotQuantityToSlot(int32 SourceSlotIndex, int32 TargetSlotIndex, int32 Quantity)
{
	if (Items.Num() != GetMaxSlotCount())
	{
		InitializeInventory();
	}

	if (!Items.IsValidIndex(SourceSlotIndex) || !Items.IsValidIndex(TargetSlotIndex) || SourceSlotIndex == TargetSlotIndex || Quantity <= 0)
	{
		return false;
	}

	const bool bMoved = FPDItemSlotTransfer::MoveQuantity(Items[SourceSlotIndex], Items[TargetSlotIndex], Quantity);
	if (bMoved)
	{
		OnInventoryChanged.Broadcast();
	}

	return bMoved;
}

int32 UPDInventoryComponent::AddItemPartial(const FPDItemData& ItemData, int32 Quantity)
{
	if (ItemData.ItemID.IsNone() || Quantity <= 0)
	{
		return 0;
	}

	int32 RemainingQuantity = Quantity;
	int32 AddedQuantity = 0;

	const int32 MaxStack = FMath::Max(1, ItemData.MaxStack);

	if (MaxStack > 1)
	{
		for (FPDInventorySlot& Slot : Items)
		{
			if (!Slot.IsEmpty() &&
				Slot.ItemData.ItemID == ItemData.ItemID &&
				Slot.Quantity < MaxStack)
			{
				const int32 AddAmount = FMath::Min(RemainingQuantity, MaxStack - Slot.Quantity);

				Slot.Quantity += AddAmount;
				RemainingQuantity -= AddAmount;
				AddedQuantity += AddAmount;

				if (RemainingQuantity <= 0)
				{
					OnInventoryChanged.Broadcast();
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

		Items[EmptySlot].ItemData = ItemData;
		Items[EmptySlot].Quantity = AddAmount;
		Items[EmptySlot].bIsEmpty = false;

		RemainingQuantity -= AddAmount;
		AddedQuantity += AddAmount;
	}

	if (AddedQuantity > 0)
	{
		OnInventoryChanged.Broadcast();
	}

	return AddedQuantity;
}

void UPDInventoryComponent::InitializeInventory()
{
	const int32 MaxSlotCount = GetMaxSlotCount();

	if (MaxSlotCount <= 0)
	{
		Items.Empty();
		OnInventoryChanged.Broadcast();
		return;
	}

	const int32 OldCount = Items.Num();

	Items.SetNum(MaxSlotCount);

	for (int32 Index = OldCount; Index < Items.Num(); ++Index)
	{
		Items[Index].Clear();
	}

	OnInventoryChanged.Broadcast();
}

void UPDInventoryComponent::ResetInventory()
{
	Items.SetNum(GetMaxSlotCount());

	for (FPDInventorySlot& Slot : Items)
	{
		Slot.Clear();
	}

	OnInventoryChanged.Broadcast();
}