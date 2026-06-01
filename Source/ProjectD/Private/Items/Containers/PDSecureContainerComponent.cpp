#include "Items/Containers/PDSecureContainerComponent.h"

#include "Core/PDGameInstance.h"
#include "Items/Containers/PDInventoryComponent.h"
#include "Items/Data/PDItemSlotTransfer.h"
#include "Items/Data/PDItemSoundLibrary.h"

UPDSecureContainerComponent::UPDSecureContainerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPDSecureContainerComponent::BeginPlay()
{
	Super::BeginPlay();
	RestoreSecureContainer();
}

float UPDSecureContainerComponent::GetCurrentWeight() const
{
	float TotalWeight = 0.f;

	for (const FPDInventorySlot& Slot : SecureItems)
	{
		if (Slot.IsEmpty())
		{
			continue;
		}

		TotalWeight += FMath::Max(0.f, Slot.ItemData.Weight) * FMath::Max(0, Slot.Quantity);
	}

	return TotalWeight;
}

const FPDInventorySlot* UPDSecureContainerComponent::GetSecureSlot(int32 SlotIndex) const
{
	return SecureItems.IsValidIndex(SlotIndex) ? &SecureItems[SlotIndex] : nullptr;
}

void UPDSecureContainerComponent::InitializeSecureContainer()
{
	const int32 OldCount = SecureItems.Num();
	ResizeSecureItems();

	if (OldCount != SecureItems.Num())
	{
		NotifySecureContainerChanged();
	}
}

void UPDSecureContainerComponent::SaveSecureContainer()
{
	if (UPDGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance<UPDGameInstance>() : nullptr)
	{
		GI->SetSecureContainerItems(SecureItems);
	}
}

bool UPDSecureContainerComponent::MoveSlotQuantityToSlot(int32 SourceSlotIndex, int32 TargetSlotIndex, int32 Quantity)
{
	ResizeSecureItems();

	if (!SecureItems.IsValidIndex(SourceSlotIndex) || !SecureItems.IsValidIndex(TargetSlotIndex) || SourceSlotIndex == TargetSlotIndex || Quantity <= 0)
	{
		return false;
	}

	const FPDItemData MovedItemData = SecureItems[SourceSlotIndex].ItemData;
	const bool bMoved = FPDItemSlotTransfer::MoveQuantity(SecureItems[SourceSlotIndex], SecureItems[TargetSlotIndex], Quantity);
	if (bMoved)
	{
		UPDItemSoundLibrary::PlayItemMoveSound(this, MovedItemData);
		NotifySecureContainerChanged();
	}

	return bMoved;
}

bool UPDSecureContainerComponent::StoreInventorySlotQuantityToSlot(UPDInventoryComponent* SourceInventory, int32 SourceSlotIndex, int32 TargetSecureSlotIndex, int32 Quantity)
{
	if (!SourceInventory || Quantity <= 0)
	{
		return false;
	}

	ResizeSecureItems();

	if (!SourceInventory->Items.IsValidIndex(SourceSlotIndex) || !SecureItems.IsValidIndex(TargetSecureSlotIndex))
	{
		return false;
	}

	FPDInventorySlot& SourceSlot = SourceInventory->Items[SourceSlotIndex];
	FPDInventorySlot& TargetSlot = SecureItems[TargetSecureSlotIndex];

	if (SourceSlot.IsEmpty())
	{
		return false;
	}

	const FPDItemData MovedItemData = SourceSlot.ItemData;
	const bool bMoved = FPDItemSlotTransfer::MoveQuantity(SourceSlot, TargetSlot, Quantity);
	if (!bMoved)
	{
		return false;
	}

	UPDItemSoundLibrary::PlayItemMoveSound(this, MovedItemData);
	NotifySecureContainerChanged();
	SourceInventory->OnInventoryChanged.Broadcast();
	return true;
}

bool UPDSecureContainerComponent::TakeSecureSlotQuantityToInventorySlot(UPDInventoryComponent* TargetInventory, int32 SecureSlotIndex, int32 TargetInventorySlotIndex, int32 Quantity)
{
	if (!TargetInventory || Quantity <= 0)
	{
		return false;
	}

	ResizeSecureItems();

	if (!SecureItems.IsValidIndex(SecureSlotIndex) || !TargetInventory->Items.IsValidIndex(TargetInventorySlotIndex))
	{
		return false;
	}

	FPDInventorySlot& SourceSlot = SecureItems[SecureSlotIndex];
	if (SourceSlot.IsEmpty())
	{
		return false;
	}

	const FPDItemData MovedItemData = SourceSlot.ItemData;
	const bool bMoved = FPDItemSlotTransfer::MoveQuantity(SourceSlot, TargetInventory->Items[TargetInventorySlotIndex], Quantity);
	if (!bMoved)
	{
		return false;
	}

	UPDItemSoundLibrary::PlayItemMoveSound(this, MovedItemData);
	NotifySecureContainerChanged();
	TargetInventory->OnInventoryChanged.Broadcast();
	return true;
}

void UPDSecureContainerComponent::RestoreSecureContainer()
{
	SecureItems.Reset();

	if (const UPDGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance<UPDGameInstance>() : nullptr)
	{
		SecureItems = GI->GetSecureContainerItems();
	}

	ResizeSecureItems();
	NotifySecureContainerChanged(false);
}

void UPDSecureContainerComponent::ResizeSecureItems()
{
	const int32 DesiredSlotCount = GetSlotCount();
	const int32 OldCount = SecureItems.Num();

	SecureItems.SetNum(DesiredSlotCount);

	for (int32 Index = OldCount; Index < SecureItems.Num(); ++Index)
	{
		SecureItems[Index].Clear();
	}
}

void UPDSecureContainerComponent::NotifySecureContainerChanged(bool bSaveToPlayerData)
{
	if (bSaveToPlayerData)
	{
		SaveSecureContainer();
	}

	OnSecureContainerChanged.Broadcast();
}
