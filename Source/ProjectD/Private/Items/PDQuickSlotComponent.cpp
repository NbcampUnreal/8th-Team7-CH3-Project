#include "Items/PDQuickSlotComponent.h"

#include "Items/PDInventoryComponent.h"
#include "Items/PDItemSlotTransfer.h"
#include "Items/PDStashComponent.h"

UPDQuickSlotComponent::UPDQuickSlotComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPDQuickSlotComponent::BeginPlay()
{
	Super::BeginPlay();

	InitializeQuickSlots();
}

int32 UPDQuickSlotComponent::FindEmptySlot() const
{
	for (int32 Index = 0; Index < QuickSlotItems.Num(); ++Index)
	{
		if (QuickSlotItems[Index].IsEmpty())
		{
			return Index;
		}
	}

	return INDEX_NONE;
}

void UPDQuickSlotComponent::InitializeQuickSlots()
{
	const int32 MaxSlotCount = GetMaxSlotCount();

	if (MaxSlotCount <= 0)
	{
		QuickSlotItems.Empty();
		OnQuickSlotsChanged.Broadcast();
		return;
	}

	const int32 OldCount = QuickSlotItems.Num();

	QuickSlotItems.SetNum(MaxSlotCount);

	for (int32 Index = OldCount; Index < QuickSlotItems.Num(); ++Index)
	{
		QuickSlotItems[Index].Clear();
	}

	for (FPDInventorySlot& Slot : QuickSlotItems)
	{
		if (Slot.IsEmpty())
		{
			Slot.Clear();
		}
	}

	OnQuickSlotsChanged.Broadcast();
}

void UPDQuickSlotComponent::ResetQuickSlots()
{
	QuickSlotItems.SetNum(GetMaxSlotCount());

	for (FPDInventorySlot& Slot : QuickSlotItems)
	{
		Slot.Clear();
	}

	OnQuickSlotsChanged.Broadcast();
}

int32 UPDQuickSlotComponent::AddItemPartial(const FPDItemData& ItemData, int32 Quantity)
{
	if (ItemData.ItemID.IsNone() || Quantity <= 0)
	{
		return 0;
	}

	if (QuickSlotItems.Num() != GetMaxSlotCount())
	{
		InitializeQuickSlots();
	}

	int32 RemainingQuantity = Quantity;
	int32 AddedQuantity = 0;

	const int32 MaxStack = FMath::Max(1, ItemData.MaxStack);

	if (MaxStack > 1)
	{
		for (FPDInventorySlot& Slot : QuickSlotItems)
		{
			if (!Slot.IsEmpty() && Slot.ItemData.ItemID == ItemData.ItemID && Slot.Quantity < MaxStack)
			{
				const int32 AddAmount = FMath::Min(RemainingQuantity, MaxStack - Slot.Quantity);
				Slot.Quantity += AddAmount;
				RemainingQuantity -= AddAmount;
				AddedQuantity += AddAmount;

				if (RemainingQuantity <= 0)
				{
					OnQuickSlotsChanged.Broadcast();
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

		QuickSlotItems[EmptySlot].ItemData = ItemData;
		QuickSlotItems[EmptySlot].Quantity = AddAmount;
		QuickSlotItems[EmptySlot].bIsEmpty = false;

		RemainingQuantity -= AddAmount;
		AddedQuantity += AddAmount;
	}

	if (AddedQuantity > 0)
	{
		OnQuickSlotsChanged.Broadcast();
	}

	return AddedQuantity;
}

int32 UPDQuickSlotComponent::AddItemToSlotPartial(const FPDItemData& ItemData, int32 Quantity, int32 TargetSlotIndex)
{
	if (QuickSlotItems.Num() != GetMaxSlotCount())
	{
		InitializeQuickSlots();
	}

	if (!QuickSlotItems.IsValidIndex(TargetSlotIndex))
	{
		return 0;
	}

	const int32 AddedQuantity = FPDItemSlotTransfer::AddItemToSlot(QuickSlotItems[TargetSlotIndex], ItemData, Quantity);
	if (AddedQuantity > 0)
	{
		OnQuickSlotsChanged.Broadcast();
	}

	return AddedQuantity;
}

bool UPDQuickSlotComponent::MoveSlotQuantityToSlot(int32 SourceSlotIndex, int32 TargetSlotIndex, int32 Quantity)
{
	if (QuickSlotItems.Num() != GetMaxSlotCount())
	{
		InitializeQuickSlots();
	}

	if (!QuickSlotItems.IsValidIndex(SourceSlotIndex) || !QuickSlotItems.IsValidIndex(TargetSlotIndex) || SourceSlotIndex == TargetSlotIndex || Quantity <= 0)
	{
		return false;
	}

	const bool bMoved = FPDItemSlotTransfer::MoveQuantity(QuickSlotItems[SourceSlotIndex], QuickSlotItems[TargetSlotIndex], Quantity);
	if (bMoved)
	{
		OnQuickSlotsChanged.Broadcast();
	}

	return bMoved;
}

bool UPDQuickSlotComponent::StoreInventorySlotQuantityToSlot(UPDInventoryComponent* SourceInventory, int32 SourceSlotIndex, int32 TargetQuickSlotIndex, int32 Quantity)
{
	if (!SourceInventory || SourceInventory->Items.Num() != SourceInventory->GetMaxSlotCount())
	{
		if (SourceInventory)
		{
			SourceInventory->InitializeInventory();
		}
	}

	if (QuickSlotItems.Num() != GetMaxSlotCount())
	{
		InitializeQuickSlots();
	}

	if (!SourceInventory || !SourceInventory->Items.IsValidIndex(SourceSlotIndex) || !QuickSlotItems.IsValidIndex(TargetQuickSlotIndex) || Quantity <= 0)
	{
		return false;
	}

	const bool bMoved = FPDItemSlotTransfer::MoveQuantity(SourceInventory->Items[SourceSlotIndex], QuickSlotItems[TargetQuickSlotIndex], Quantity);
	if (bMoved)
	{
		SourceInventory->OnInventoryChanged.Broadcast();
		OnQuickSlotsChanged.Broadcast();
	}

	return bMoved;
}

bool UPDQuickSlotComponent::StoreStashSlotQuantityToSlot(UPDStashComponent* SourceStash, int32 SourceStashSlotIndex, int32 TargetQuickSlotIndex, int32 Quantity)
{
	if (!SourceStash || SourceStash->StashItems.Num() != SourceStash->GetMaxSlotCount())
	{
		if (SourceStash)
		{
			SourceStash->InitializeStash();
		}
	}

	if (QuickSlotItems.Num() != GetMaxSlotCount())
	{
		InitializeQuickSlots();
	}

	if (!SourceStash || !SourceStash->StashItems.IsValidIndex(SourceStashSlotIndex) || !QuickSlotItems.IsValidIndex(TargetQuickSlotIndex) || Quantity <= 0)
	{
		return false;
	}

	const bool bMoved = FPDItemSlotTransfer::MoveQuantity(SourceStash->StashItems[SourceStashSlotIndex], QuickSlotItems[TargetQuickSlotIndex], Quantity);
	if (bMoved)
	{
		SourceStash->OnStashChanged.Broadcast();
		OnQuickSlotsChanged.Broadcast();
	}

	return bMoved;
}

bool UPDQuickSlotComponent::TakeQuickSlotQuantityToInventorySlot(UPDInventoryComponent* TargetInventory, int32 QuickSlotIndex, int32 TargetInventorySlotIndex, int32 Quantity)
{
	if (!TargetInventory || TargetInventory->Items.Num() != TargetInventory->GetMaxSlotCount())
	{
		if (TargetInventory)
		{
			TargetInventory->InitializeInventory();
		}
	}

	if (QuickSlotItems.Num() != GetMaxSlotCount())
	{
		InitializeQuickSlots();
	}

	if (!TargetInventory || !QuickSlotItems.IsValidIndex(QuickSlotIndex) || !TargetInventory->Items.IsValidIndex(TargetInventorySlotIndex) || Quantity <= 0)
	{
		return false;
	}

	const bool bMoved = FPDItemSlotTransfer::MoveQuantity(QuickSlotItems[QuickSlotIndex], TargetInventory->Items[TargetInventorySlotIndex], Quantity);
	if (bMoved)
	{
		OnQuickSlotsChanged.Broadcast();
		TargetInventory->OnInventoryChanged.Broadcast();
	}

	return bMoved;
}

bool UPDQuickSlotComponent::TakeQuickSlotQuantityToStashSlot(UPDStashComponent* TargetStash, int32 QuickSlotIndex, int32 TargetStashSlotIndex, int32 Quantity)
{
	if (!TargetStash || TargetStash->StashItems.Num() != TargetStash->GetMaxSlotCount())
	{
		if (TargetStash)
		{
			TargetStash->InitializeStash();
		}
	}

	if (QuickSlotItems.Num() != GetMaxSlotCount())
	{
		InitializeQuickSlots();
	}

	if (!TargetStash || !QuickSlotItems.IsValidIndex(QuickSlotIndex) || !TargetStash->StashItems.IsValidIndex(TargetStashSlotIndex) || Quantity <= 0)
	{
		return false;
	}

	const bool bMoved = FPDItemSlotTransfer::MoveQuantity(QuickSlotItems[QuickSlotIndex], TargetStash->StashItems[TargetStashSlotIndex], Quantity);
	if (bMoved)
	{
		OnQuickSlotsChanged.Broadcast();
		TargetStash->OnStashChanged.Broadcast();
	}

	return bMoved;
}

bool UPDQuickSlotComponent::RemoveItemFromSlot(int32 SlotIndex, int32 Quantity)
{
	if (!QuickSlotItems.IsValidIndex(SlotIndex) || Quantity <= 0)
	{
		return false;
	}

	FPDInventorySlot& Slot = QuickSlotItems[SlotIndex];
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

	OnQuickSlotsChanged.Broadcast();
	return true;
}

bool UPDQuickSlotComponent::HasItem(FName ItemID, int32 Quantity) const
{
	if (ItemID.IsNone() || Quantity <= 0)
	{
		return false;
	}

	int32 FoundQuantity = 0;

	for (const FPDInventorySlot& Slot : QuickSlotItems)
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
