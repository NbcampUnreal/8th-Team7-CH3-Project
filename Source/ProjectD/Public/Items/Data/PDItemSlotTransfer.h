#pragma once

#include "CoreMinimal.h"
#include "Type/Types.h"

struct PROJECTD_API FPDItemSlotTransfer
{
	static int32 AddItemToSlot(FPDInventorySlot& TargetSlot, const FPDItemData& ItemData, int32 Quantity)
	{
		if (ItemData.ItemID.IsNone() || Quantity <= 0)
		{
			return 0;
		}

		const int32 MaxStack = FMath::Max(1, ItemData.MaxStack);

		if (TargetSlot.IsEmpty())
		{
			const int32 AddAmount = FMath::Min(Quantity, MaxStack);
			TargetSlot.ItemData = ItemData;
			TargetSlot.Quantity = AddAmount;
			TargetSlot.bIsEmpty = false;
			TargetSlot.ModificationLevel = 0;
			TargetSlot.WeaponState.Reset();
			TargetSlot.AssignNewInstanceID();
			return AddAmount;
		}

		if (TargetSlot.ItemData.ItemID != ItemData.ItemID)
		{
			return 0;
		}

		const int32 Capacity = MaxStack - TargetSlot.Quantity;
		if (Capacity <= 0)
		{
			return 0;
		}

		const int32 AddAmount = FMath::Min(Quantity, Capacity);
		TargetSlot.Quantity += AddAmount;
		TargetSlot.bIsEmpty = false;
		TargetSlot.EnsureInstanceID();
		return AddAmount;
	}

	static int32 GetQuantityLimit(const FPDInventorySlot& SourceSlot, const FPDInventorySlot& TargetSlot)
	{
		if (SourceSlot.IsEmpty())
		{
			return 0;
		}

		const int32 SourceQuantity = FMath::Max(0, SourceSlot.Quantity);
		const int32 MaxStack = FMath::Max(1, SourceSlot.ItemData.MaxStack);

		if (TargetSlot.IsEmpty())
		{
			return FMath::Min(SourceQuantity, MaxStack);
		}

		if (TargetSlot.ItemData.ItemID == SourceSlot.ItemData.ItemID)
		{
			return FMath::Clamp(MaxStack - TargetSlot.Quantity, 0, SourceQuantity);
		}

		return SourceQuantity;
	}

	static bool MoveQuantity(FPDInventorySlot& SourceSlot, FPDInventorySlot& TargetSlot, int32 Quantity)
	{
		if (SourceSlot.IsEmpty() || Quantity <= 0)
		{
			return false;
		}

		const int32 MoveQuantity = FMath::Min(Quantity, SourceSlot.Quantity);
		if (MoveQuantity <= 0)
		{
			return false;
		}

		if (TargetSlot.IsEmpty())
		{
			if (MoveQuantity >= SourceSlot.Quantity)
			{
				TargetSlot = SourceSlot;
				TargetSlot.EnsureInstanceID();
				SourceSlot.Clear();
				return true;
			}

			TargetSlot.ItemData = SourceSlot.ItemData;
			TargetSlot.Quantity = MoveQuantity;
			TargetSlot.bIsEmpty = false;
			TargetSlot.ModificationLevel = SourceSlot.ModificationLevel;
			TargetSlot.WeaponState = SourceSlot.WeaponState;
			TargetSlot.AssignNewInstanceID();
			SourceSlot.Quantity -= MoveQuantity;
			return true;
		}

		if (TargetSlot.ItemData.ItemID == SourceSlot.ItemData.ItemID)
		{
			const int32 MaxStack = FMath::Max(1, SourceSlot.ItemData.MaxStack);
			const int32 Capacity = MaxStack - TargetSlot.Quantity;
			if (Capacity <= 0)
			{
				return false;
			}

			const int32 AddAmount = FMath::Min(MoveQuantity, Capacity);
			TargetSlot.Quantity += AddAmount;
			SourceSlot.Quantity -= AddAmount;

			if (SourceSlot.Quantity <= 0)
			{
				SourceSlot.Clear();
			}

			return AddAmount > 0;
		}

		if (MoveQuantity >= SourceSlot.Quantity)
		{
			Swap(SourceSlot, TargetSlot);
			SourceSlot.EnsureInstanceID();
			TargetSlot.EnsureInstanceID();
			return true;
		}

		return false;
	}

	static bool CanPromptForPartial(const FPDInventorySlot& SourceSlot, const FPDInventorySlot& TargetSlot)
	{
		if (SourceSlot.IsEmpty() || SourceSlot.Quantity <= 1 || SourceSlot.ItemData.MaxStack <= 1)
		{
			return false;
		}

		return TargetSlot.IsEmpty() || TargetSlot.ItemData.ItemID == SourceSlot.ItemData.ItemID;
	}
};

struct PROJECTD_API FPDItemContainerOps
{
	static void EnsureInstanceIDs(TArray<FPDInventorySlot>& Slots)
	{
		for (FPDInventorySlot& Slot : Slots)
		{
			Slot.EnsureInstanceID();
		}
	}

	static int32 FindEmptySlot(const TArray<FPDInventorySlot>& Slots)
	{
		for (int32 Index = 0; Index < Slots.Num(); ++Index)
		{
			if (Slots[Index].IsEmpty())
			{
				return Index;
			}
		}

		return INDEX_NONE;
	}

	static int32 CountItem(const TArray<FPDInventorySlot>& Slots, FName ItemID)
	{
		if (ItemID.IsNone())
		{
			return 0;
		}

		int32 TotalQuantity = 0;
		for (const FPDInventorySlot& Slot : Slots)
		{
			if (!Slot.IsEmpty() && Slot.ItemData.ItemID == ItemID)
			{
				TotalQuantity += Slot.Quantity;
			}
		}

		return TotalQuantity;
	}

	static bool HasItem(const TArray<FPDInventorySlot>& Slots, FName ItemID, int32 Quantity)
	{
		return Quantity > 0 && CountItem(Slots, ItemID) >= Quantity;
	}

	static int32 AddItem(TArray<FPDInventorySlot>& Slots, const FPDItemData& ItemData, int32 Quantity)
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
			for (FPDInventorySlot& Slot : Slots)
			{
				if (RemainingQuantity <= 0)
				{
					break;
				}

				if (!Slot.IsEmpty() && Slot.ItemData.ItemID == ItemData.ItemID && Slot.Quantity < MaxStack)
				{
					const int32 AddedToSlot = FPDItemSlotTransfer::AddItemToSlot(Slot, ItemData, RemainingQuantity);
					RemainingQuantity -= AddedToSlot;
					AddedQuantity += AddedToSlot;
				}
			}
		}

		while (RemainingQuantity > 0)
		{
			const int32 EmptySlotIndex = FindEmptySlot(Slots);
			if (EmptySlotIndex == INDEX_NONE)
			{
				break;
			}

			const int32 AddedToSlot = FPDItemSlotTransfer::AddItemToSlot(Slots[EmptySlotIndex], ItemData, RemainingQuantity);
			if (AddedToSlot <= 0)
			{
				break;
			}

			RemainingQuantity -= AddedToSlot;
			AddedQuantity += AddedToSlot;
		}

		return AddedQuantity;
	}

	static int32 AddSlot(TArray<FPDInventorySlot>& Slots, const FPDInventorySlot& SourceSlot)
	{
		if (SourceSlot.IsEmpty() || SourceSlot.Quantity <= 0)
		{
			return 0;
		}

		if (SourceSlot.ItemData.ItemType == EPDItemType::Equipment ||
			SourceSlot.ItemData.WeaponType != EWeaponType::None ||
			SourceSlot.ModificationLevel > 0 ||
			SourceSlot.ItemData.MaxStack <= 1)
		{
			const int32 EmptySlotIndex = FindEmptySlot(Slots);
			if (EmptySlotIndex == INDEX_NONE)
			{
				return 0;
			}

			Slots[EmptySlotIndex] = SourceSlot;
			Slots[EmptySlotIndex].Quantity = FMath::Max(1, SourceSlot.Quantity);
			Slots[EmptySlotIndex].bIsEmpty = false;
			Slots[EmptySlotIndex].EnsureInstanceID();
			return Slots[EmptySlotIndex].Quantity;
		}

		return AddItem(Slots, SourceSlot.ItemData, SourceSlot.Quantity);
	}

	static bool RemoveItem(TArray<FPDInventorySlot>& Slots, FName ItemID, int32 Quantity)
	{
		if (ItemID.IsNone() || Quantity <= 0 || !HasItem(Slots, ItemID, Quantity))
		{
			return false;
		}

		int32 RemainingQuantity = Quantity;
		for (int32 Index = Slots.Num() - 1; Index >= 0 && RemainingQuantity > 0; --Index)
		{
			FPDInventorySlot& Slot = Slots[Index];
			if (Slot.IsEmpty() || Slot.ItemData.ItemID != ItemID)
			{
				continue;
			}

			const int32 RemoveAmount = FMath::Min(RemainingQuantity, Slot.Quantity);
			Slot.Quantity -= RemoveAmount;
			RemainingQuantity -= RemoveAmount;

			if (Slot.Quantity <= 0)
			{
				Slot.Clear();
			}
		}

		return true;
	}
};
