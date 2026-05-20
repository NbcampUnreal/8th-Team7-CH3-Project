#include "Items/PDEquipmentComponent.h"

#include "Data/PDQuestComponent.h"

#include "Characters/PDPlayerCharacter.h"
#include "Weapons/Base/PDRangedWeaponBase.h"
#include "Items/PDInventoryComponent.h"
#include "Items/PDItemSoundLibrary.h"
#include "Net/UnrealNetwork.h"

UPDEquipmentComponent::UPDEquipmentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UPDEquipmentComponent::BeginPlay()
{
	Super::BeginPlay();
	InitializeDefaultSlots();
}

void UPDEquipmentComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UPDEquipmentComponent, ReplicatedEquippedItems);
}

void UPDEquipmentComponent::InitializeDefaultSlots()
{
	const EPDEquipmentSlotType DefaultSlots[] =
	{
		EPDEquipmentSlotType::Weapon,
		EPDEquipmentSlotType::Head,
		EPDEquipmentSlotType::Armor,
		EPDEquipmentSlotType::Bag,
	};

	for (EPDEquipmentSlotType SlotType : DefaultSlots)
	{
		if (!EquippedItems.Contains(SlotType))
		{
			FPDEquippedItem EmptyItem;
			EmptyItem.SlotType = SlotType;
			EquippedItems.Add(SlotType, EmptyItem);
		}
	}

	RefreshReplicatedEquippedItems();
}

EPDEquipmentSlotType UPDEquipmentComponent::ResolveEquipmentSlotType(const FPDItemData& ItemData) const
{
	if (ItemData.ItemType != EPDItemType::Equipment)
	{
		return EPDEquipmentSlotType::None;
	}

	if (ItemData.EquipmentSlotType != EPDEquipmentSlotType::None)
	{
		return ItemData.EquipmentSlotType;
	}

	if (ItemData.WeaponType != EWeaponType::None)
	{
		return EPDEquipmentSlotType::Weapon;
	}

	return EPDEquipmentSlotType::None;
}

bool UPDEquipmentComponent::CanEquipItem(const FPDItemData& ItemData) const
{
	return ResolveEquipmentSlotType(ItemData) != EPDEquipmentSlotType::None;
}

bool UPDEquipmentComponent::IsSlotOccupied(EPDEquipmentSlotType SlotType) const
{
	const FPDEquippedItem* EquippedItem = EquippedItems.Find(SlotType);
	return EquippedItem && !EquippedItem->ItemSlot.IsEmpty();
}

FPDInventorySlot UPDEquipmentComponent::GetEquippedSlot(EPDEquipmentSlotType SlotType) const
{
	if (const FPDEquippedItem* EquippedItem = EquippedItems.Find(SlotType))
	{
		return EquippedItem->ItemSlot;
	}

	return FPDInventorySlot();
}

bool UPDEquipmentComponent::EquipItemFromInventory(UPDInventoryComponent* InventoryComponent, int32 InventorySlotIndex)
{
	if (!InventoryComponent || !InventoryComponent->Items.IsValidIndex(InventorySlotIndex))
	{
		return false;
	}

	const FPDInventorySlot InventorySlot = InventoryComponent->Items[InventorySlotIndex];
	if (InventorySlot.IsEmpty())
	{
		return false;
	}

	return EquipItemFromInventoryToSlot(InventoryComponent, InventorySlotIndex, ResolveEquipmentSlotType(InventorySlot.ItemData));
}

bool UPDEquipmentComponent::EquipItemFromInventoryToSlot(UPDInventoryComponent* InventoryComponent, int32 InventorySlotIndex, EPDEquipmentSlotType TargetSlotType)
{
	if (AActor* Owner = GetOwner(); Owner && !Owner->HasAuthority())
	{
		if (TargetSlotType != EPDEquipmentSlotType::None)
		{
			ServerEquipItemFromInventoryToSlot(InventorySlotIndex, TargetSlotType);
			return true;
		}
		return false;
	}

	if (!InventoryComponent || !InventoryComponent->Items.IsValidIndex(InventorySlotIndex) || TargetSlotType == EPDEquipmentSlotType::None)
	{
		return false;
	}

	const FPDInventorySlot InventorySlot = InventoryComponent->Items[InventorySlotIndex];
	if (InventorySlot.IsEmpty() || InventorySlot.ItemData.ItemType != EPDItemType::Equipment || InventorySlot.Quantity != 1)
	{
		return false;
	}

	if (ResolveEquipmentSlotType(InventorySlot.ItemData) != TargetSlotType)
	{
		return false;
	}

	FPDEquippedItem* CurrentEquippedItem = EquippedItems.Find(TargetSlotType);
	FPDInventorySlot PreviousEquippedSlot;
	const bool bHadPreviousItem = CurrentEquippedItem && !CurrentEquippedItem->ItemSlot.IsEmpty();
	if (bHadPreviousItem)
	{
		PreviousEquippedSlot = CurrentEquippedItem->ItemSlot;
	}

	FPDWeaponInstanceState CapturedPreviousState;

	const float NewBagCapacityWeight = TargetSlotType == EPDEquipmentSlotType::Bag ? FMath::Max(0.f, InventorySlot.ItemData.BagCapacityWeight) : [&]()
	{
		const FPDInventorySlot CurrentBagSlot = GetEquippedSlot(EPDEquipmentSlotType::Bag);
		return CurrentBagSlot.IsEmpty() ? 0.f : FMath::Max(0.f, CurrentBagSlot.ItemData.BagCapacityWeight);
	}();

	if (!InventoryComponent->CanFitWeightAfterEquipmentChange(InventorySlot, PreviousEquippedSlot, NewBagCapacityWeight))
	{
		InventoryComponent->BroadcastWeightLimitExceeded();
		return false;
	}

	if (bHadPreviousItem)
	{
		RemoveCharacterEquipSideEffects(PreviousEquippedSlot, &CapturedPreviousState);
		// 추출된 상태를 인벤토리로 반환될 슬롯에 stamp — 이후 분기에서 그대로 사용됨.
		PreviousEquippedSlot.WeaponState = CapturedPreviousState;
	}

	if (!ApplyCharacterEquipSideEffects(InventorySlot))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[Equip] ApplyCharacterEquipSideEffects failed for %s. Rolling back to %s."),
			*InventorySlot.ItemData.ItemID.ToString(),
			bHadPreviousItem ? *PreviousEquippedSlot.ItemData.ItemID.ToString() : TEXT("(none)"));

		if (bHadPreviousItem)
		{
			// 롤백: 캡처한 상태를 함께 복원해 잔탄 손실 방지.
			ApplyCharacterEquipSideEffects(PreviousEquippedSlot);
		}
		return false;
	}

	FPDInventorySlot EquippedSlot = InventorySlot;
	EquippedSlot.Quantity = 1;
	EquippedSlot.bIsEmpty = false;

	FPDEquippedItem NewEquippedItem;
	NewEquippedItem.SlotType = TargetSlotType;
	NewEquippedItem.ItemSlot = EquippedSlot;
	EquippedItems.Add(TargetSlotType, NewEquippedItem);

	if (bHadPreviousItem)
	{
		InventoryComponent->Items[InventorySlotIndex] = PreviousEquippedSlot;
		InventoryComponent->Items[InventorySlotIndex].Quantity = FMath::Max(1, InventoryComponent->Items[InventorySlotIndex].Quantity);
		InventoryComponent->Items[InventorySlotIndex].bIsEmpty = false;
	}
	else
	{
		InventoryComponent->Items[InventorySlotIndex].Clear();
	}

	UPDItemSoundLibrary::PlayItemMoveSound(this, EquippedSlot.ItemData);
	InventoryComponent->OnInventoryChanged.Broadcast();
	BroadcastSlotChanged(TargetSlotType);
	BroadcastModificationApplied(TargetSlotType, EquippedSlot);
	RefreshReplicatedEquippedItems();
	return true;
}

void UPDEquipmentComponent::ServerEquipItemFromInventoryToSlot_Implementation(int32 InventorySlotIndex, EPDEquipmentSlotType TargetSlotType)
{
	UPDInventoryComponent* InventoryComponent = GetOwner() ? GetOwner()->FindComponentByClass<UPDInventoryComponent>() : nullptr;
	EquipItemFromInventoryToSlot(InventoryComponent, InventorySlotIndex, TargetSlotType);
}

void UPDEquipmentComponent::OnRep_ReplicatedEquippedItems()
{
	EquippedItems.Empty();
	for (const FPDEquippedItem& EquippedItem : ReplicatedEquippedItems)
	{
		EquippedItems.Add(EquippedItem.SlotType, EquippedItem);
	}

	OnEquipmentChanged.Broadcast();
	for (const TPair<EPDEquipmentSlotType, FPDEquippedItem>& Pair : EquippedItems)
	{
		OnEquipmentSlotChanged.Broadcast(Pair.Key, Pair.Value.ItemSlot);
	}
}

void UPDEquipmentComponent::RefreshReplicatedEquippedItems()
{
	if (AActor* Owner = GetOwner(); Owner && !Owner->HasAuthority())
	{
		return;
	}

	ReplicatedEquippedItems.Reset();
	for (const TPair<EPDEquipmentSlotType, FPDEquippedItem>& Pair : EquippedItems)
	{
		ReplicatedEquippedItems.Add(Pair.Value);
	}
}


bool UPDEquipmentComponent::TryEquipNewItem(const FPDItemData& ItemData, bool bReportQuestProgress)
{
	const EPDEquipmentSlotType TargetSlotType = ResolveEquipmentSlotType(ItemData);
	if (TargetSlotType == EPDEquipmentSlotType::None || IsSlotOccupied(TargetSlotType))
	{
		return false;
	}

	FPDInventorySlot NewSlot;
	NewSlot.ItemData = ItemData;
	NewSlot.Quantity = 1;
	NewSlot.bIsEmpty = false;
	NewSlot.ModificationLevel = 0;

	if (!ApplyCharacterEquipSideEffects(NewSlot))
	{
		return false;
	}

	FPDEquippedItem NewEquippedItem;
	NewEquippedItem.SlotType = TargetSlotType;
	NewEquippedItem.ItemSlot = NewSlot;
	EquippedItems.Add(TargetSlotType, NewEquippedItem);

	UPDItemSoundLibrary::PlayItemMoveSound(this, NewSlot.ItemData);
	if (bReportQuestProgress)
	{
		if (UPDQuestComponent* QuestComponent = GetOwner() ? GetOwner()->FindComponentByClass<UPDQuestComponent>() : nullptr)
		{
			QuestComponent->ReportItemAcquired(ItemData.ItemID, 1);
			if (ItemData.bIsQuestItem)
			{
				QuestComponent->ReportQuestItemAcquired(ItemData.ItemID, 1);
			}
		}
	}
	BroadcastSlotChanged(TargetSlotType);
	BroadcastModificationApplied(TargetSlotType, NewSlot);
	RefreshReplicatedEquippedItems();
	return true;
}

bool UPDEquipmentComponent::UnequipItemToInventory(UPDInventoryComponent* InventoryComponent, EPDEquipmentSlotType SlotType)
{
	if (!InventoryComponent || SlotType == EPDEquipmentSlotType::None)
	{
		return false;
	}

	FPDEquippedItem* EquippedItem = EquippedItems.Find(SlotType);
	if (!EquippedItem || EquippedItem->ItemSlot.IsEmpty())
	{
		return false;
	}

	FPDInventorySlot ItemToReturn = EquippedItem->ItemSlot;

	// ★ Destroy 전에 무기 액터에서 런타임 상태 캡처 — 인벤토리로 돌려보낼 슬롯에 stamp.
	if (APDPlayerCharacter* PC = Cast<APDPlayerCharacter>(GetOwner()))
	{
		const EWeaponSlot Ts = PC->GetSlotForWeaponType(ItemToReturn.ItemData.WeaponType);
		if (APDRangedWeaponBase* Ranged = Cast<APDRangedWeaponBase>(PC->GetWeaponInSlot(Ts)))
		{
			ItemToReturn.WeaponState.CurrentAmmo = Ranged->GetCurrentAmmo();
		}
	}

	const float NewBagCapacityWeight = SlotType == EPDEquipmentSlotType::Bag ? 0.f : [&]()
	{
		const FPDInventorySlot CurrentBagSlot = GetEquippedSlot(EPDEquipmentSlotType::Bag);
		return CurrentBagSlot.IsEmpty() ? 0.f : FMath::Max(0.f, CurrentBagSlot.ItemData.BagCapacityWeight);
	}();

	if (!InventoryComponent->CanFitWeightAfterEquipmentChange(FPDInventorySlot(), ItemToReturn, NewBagCapacityWeight))
	{
		InventoryComponent->BroadcastWeightLimitExceeded();
		return false;
	}

	const int32 AddedQuantity = InventoryComponent->AddSlotPartial(ItemToReturn);
	if (AddedQuantity != ItemToReturn.Quantity)
	{
		return false;
	}

	RemoveCharacterEquipSideEffects(ItemToReturn);
	EquippedItem->ItemSlot.Clear();
	BroadcastSlotChanged(SlotType);
	RefreshReplicatedEquippedItems();
	return true;
}


bool UPDEquipmentComponent::UnequipItemToInventorySlot(UPDInventoryComponent* InventoryComponent, EPDEquipmentSlotType SlotType, int32 InventorySlotIndex)
{
	if (!InventoryComponent || SlotType == EPDEquipmentSlotType::None || !InventoryComponent->Items.IsValidIndex(InventorySlotIndex))
	{
		return false;
	}

	FPDEquippedItem* EquippedItem = EquippedItems.Find(SlotType);
	if (!EquippedItem || EquippedItem->ItemSlot.IsEmpty())
	{
		return false;
	}

	FPDInventorySlot ItemToReturn = EquippedItem->ItemSlot;

	// ★ Destroy 전에 무기 액터에서 런타임 상태 캡처.
	if (APDPlayerCharacter* PC = Cast<APDPlayerCharacter>(GetOwner()))
	{
		const EWeaponSlot Ts = PC->GetSlotForWeaponType(ItemToReturn.ItemData.WeaponType);
		if (APDRangedWeaponBase* Ranged = Cast<APDRangedWeaponBase>(PC->GetWeaponInSlot(Ts)))
		{
			ItemToReturn.WeaponState.CurrentAmmo = Ranged->GetCurrentAmmo();
		}
	}

	const float NewBagCapacityWeight = SlotType == EPDEquipmentSlotType::Bag ? 0.f : [&]()
	{
		const FPDInventorySlot CurrentBagSlot = GetEquippedSlot(EPDEquipmentSlotType::Bag);
		return CurrentBagSlot.IsEmpty() ? 0.f : FMath::Max(0.f, CurrentBagSlot.ItemData.BagCapacityWeight);
	}();

	if (!InventoryComponent->CanFitWeightAfterEquipmentChange(FPDInventorySlot(), ItemToReturn, NewBagCapacityWeight))
	{
		InventoryComponent->BroadcastWeightLimitExceeded();
		return false;
	}

	if (!InventoryComponent->Items[InventorySlotIndex].IsEmpty())
	{
		return false;
	}

	InventoryComponent->Items[InventorySlotIndex] = ItemToReturn;
	InventoryComponent->Items[InventorySlotIndex].bIsEmpty = false;
	InventoryComponent->Items[InventorySlotIndex].Quantity = FMath::Max(1, InventoryComponent->Items[InventorySlotIndex].Quantity);

	RemoveCharacterEquipSideEffects(ItemToReturn);
	EquippedItem->ItemSlot.Clear();
	UPDItemSoundLibrary::PlayItemMoveSound(this, ItemToReturn.ItemData);
	InventoryComponent->OnInventoryChanged.Broadcast();
	BroadcastSlotChanged(SlotType);
	RefreshReplicatedEquippedItems();
	return true;
}

bool UPDEquipmentComponent::ApplyCharacterEquipSideEffects(const FPDInventorySlot& ItemSlot) const
{
	const FPDItemData& ItemData = ItemSlot.ItemData;

	if (ItemData.WeaponType == EWeaponType::None)
	{
		return true;
	}

	if (APDPlayerCharacter* PlayerCharacter = Cast<APDPlayerCharacter>(GetOwner()))
	{
		return PlayerCharacter->TryAutoEquipWeaponSlot(ItemSlot);
	}

	return false;
}

void UPDEquipmentComponent::RemoveCharacterEquipSideEffects(const FPDInventorySlot& ItemSlot,
                                                              FPDWeaponInstanceState* OutWeaponState) const
{
	const FPDItemData& ItemData = ItemSlot.ItemData;
	if (ItemData.WeaponType == EWeaponType::None)
	{
		return;
	}

	if (APDPlayerCharacter* PlayerCharacter = Cast<APDPlayerCharacter>(GetOwner()))
	{
		if (OutWeaponState)
		{
			PlayerCharacter->RemoveEquippedWeaponItemPreservingState(ItemData, *OutWeaponState);
		}
		else
		{
			PlayerCharacter->RemoveEquippedWeaponItem(ItemData);
		}
	}
}

int32 UPDEquipmentComponent::ConvertModificationLevelToGasLevel(int32 ModificationLevel) const
{
	return FMath::Max(1, ModificationLevel + 1);
}

void UPDEquipmentComponent::BroadcastModificationApplied(EPDEquipmentSlotType SlotType, const FPDInventorySlot& EquippedSlot)
{
	const int32 GasLevel = ConvertModificationLevelToGasLevel(EquippedSlot.ModificationLevel);
	OnEquipmentModificationApplied.Broadcast(SlotType, EquippedSlot, GasLevel);
}

void UPDEquipmentComponent::BroadcastSlotChanged(EPDEquipmentSlotType SlotType)
{
	const FPDInventorySlot EquippedSlot = GetEquippedSlot(SlotType);
	OnEquipmentSlotChanged.Broadcast(SlotType, EquippedSlot);
	OnEquipmentChanged.Broadcast();
}
