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
				SourceSlot.Clear();
				return true;
			}

			TargetSlot.ItemData = SourceSlot.ItemData;
			TargetSlot.Quantity = MoveQuantity;
			TargetSlot.bIsEmpty = false;
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
