#include "Items/Equipment/PDEquipmentComponent.h"

#include "Characters/PDPlayerCharacter.h"
#include "Core/PDPlayerState.h"
#include "Weapons/Base/PDWeaponBase.h"
#include "Weapons/Base/PDRangedWeaponBase.h"
#include "Items/Containers/PDInventoryComponent.h"
#include "Items/Containers/PDQuickSlotComponent.h"
#include "Net/UnrealNetwork.h"

namespace
{
	void InitializeNewWeaponAmmoState(FPDInventorySlot& Slot)
	{
		if (Slot.WeaponState.HasPersistedAmmo() || Slot.ItemData.WeaponType == EWeaponType::None || !Slot.ItemData.WeaponClass)
		{
			return;
		}

		const APDRangedWeaponBase* RangedCDO = Cast<APDRangedWeaponBase>(Slot.ItemData.WeaponClass->GetDefaultObject());
		if (RangedCDO)
		{
			Slot.WeaponState.CurrentAmmo = FMath::Max(0, RangedCDO->GetMaxAmmo());
		}
	}

	APDPlayerCharacter* ResolvePlayerCharacter(AActor* Owner)
	{
		if (APDPlayerCharacter* PlayerCharacter = Cast<APDPlayerCharacter>(Owner))
		{
			return PlayerCharacter;
		}

		if (const APDPlayerState* PDPlayerState = Cast<APDPlayerState>(Owner))
		{
			return PDPlayerState->GetPDPlayerCharacter();
		}

		return nullptr;
	}

	APDWeaponBase* FindMatchingEquippedWeapon(APDPlayerCharacter* PlayerCharacter, const FPDInventorySlot& ItemSlot)
	{
		if (!PlayerCharacter || ItemSlot.ItemData.WeaponType == EWeaponType::None)
		{
			return nullptr;
		}

		const EWeaponSlot TargetSlot = PlayerCharacter->GetSlotForWeaponType(ItemSlot.ItemData.WeaponType);
		APDWeaponBase* Weapon = PlayerCharacter->GetWeaponInSlot(TargetSlot);
		if (!Weapon)
		{
			return nullptr;
		}

		if (ItemSlot.ItemInstanceID.IsValid())
		{
			return Weapon->GetItemInstanceID() == ItemSlot.ItemInstanceID ? Weapon : nullptr;
		}

		return Weapon->GetItemID() == ItemSlot.ItemData.ItemID ? Weapon : nullptr;
	}
}

UPDEquipmentComponent::UPDEquipmentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UPDEquipmentComponent::BeginPlay()
{
	Super::BeginPlay();
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		InitializeDefaultSlots();
	}
}

void UPDEquipmentComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UPDEquipmentComponent, ReplicatedEquippedItems);
}

void UPDEquipmentComponent::OnRep_EquippedItems()
{
	RebuildEquippedItemsFromReplication();
	BroadcastAllSlotsChanged();
}

void UPDEquipmentComponent::ServerEquipItemFromInventory_Implementation(UPDInventoryComponent* InventoryComponent, int32 InventorySlotIndex)
{
	EquipItemFromInventory(InventoryComponent, InventorySlotIndex);
}

void UPDEquipmentComponent::ServerEquipItemFromInventoryToSlot_Implementation(UPDInventoryComponent* InventoryComponent, int32 InventorySlotIndex, EPDEquipmentSlotType TargetSlotType)
{
	EquipItemFromInventoryToSlot(InventoryComponent, InventorySlotIndex, TargetSlotType);
}

void UPDEquipmentComponent::ServerUnequipItemToInventory_Implementation(UPDInventoryComponent* InventoryComponent, EPDEquipmentSlotType SlotType)
{
	UnequipItemToInventory(InventoryComponent, SlotType);
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
	SyncReplicatedEquippedItems();
}

void UPDEquipmentComponent::RebuildEquippedItemsFromReplication()
{
	EquippedItems.Empty();
	for (const FPDEquippedItem& EquippedItem : ReplicatedEquippedItems)
	{
		if (EquippedItem.SlotType != EPDEquipmentSlotType::None)
		{
			EquippedItems.Add(EquippedItem.SlotType, EquippedItem);
		}
	}
}

void UPDEquipmentComponent::SyncReplicatedEquippedItems()
{
	ReplicatedEquippedItems.Reset();
	for (const TPair<EPDEquipmentSlotType, FPDEquippedItem>& Pair : EquippedItems)
	{
		ReplicatedEquippedItems.Add(Pair.Value);
	}
}

EPDEquipmentSlotType UPDEquipmentComponent::ResolveEquipmentSlotType(const FPDItemData& ItemData) const
{
	if (ItemData.WeaponType != EWeaponType::None)
	{
		return EPDEquipmentSlotType::Weapon;
	}

	if (ItemData.ItemType != EPDItemType::Equipment)
	{
		return EPDEquipmentSlotType::None;
	}

	if (ItemData.EquipmentSlotType != EPDEquipmentSlotType::None)
	{
		return ItemData.EquipmentSlotType;
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
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		ServerEquipItemFromInventory(InventoryComponent, InventorySlotIndex);
		return true;
	}

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

	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		ServerEquipItemFromInventoryToSlot(InventoryComponent, InventorySlotIndex, TargetSlotType);
		return true;
	}

	if (!InventoryComponent || !InventoryComponent->Items.IsValidIndex(InventorySlotIndex) || TargetSlotType == EPDEquipmentSlotType::None)
	{
		return false;
	}

	FPDInventorySlot InventorySlot = InventoryComponent->Items[InventorySlotIndex];
	if (InventorySlot.IsEmpty() ||
		(InventorySlot.ItemData.ItemType != EPDItemType::Equipment && InventorySlot.ItemData.WeaponType == EWeaponType::None) ||
		InventorySlot.Quantity != 1)
	{
		return false;
	}
	InventorySlot.EnsureInstanceID();
	InventoryComponent->Items[InventorySlotIndex].ItemInstanceID = InventorySlot.ItemInstanceID;

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
		// Ï∂îÏ∂ú???ÅÌÉúÎ•??∏Î≤§?†Î¶¨Î°?Î∞òÌôò???¨Î°Ø??stamp ???¥ÌõÑ Î∂ÑÍ∏∞?êÏÑú Í∑∏Î?Î°??¨Ïö©??
		PreviousEquippedSlot.WeaponState = CapturedPreviousState;
	}

	if (!ApplyCharacterEquipSideEffects(InventorySlot))
	{
		if (bHadPreviousItem)
		{
			// Î°§Î∞±: Ï∫°Ï≤ò???ÅÌÉúÎ•??®Íªò Î≥µÏõê???îÌÉÑ ?êÏã§ Î∞©Ï?.
			ApplyCharacterEquipSideEffects(PreviousEquippedSlot);
		}
		return false;
	}

	FPDInventorySlot EquippedSlot = InventorySlot;
	EquippedSlot.Quantity = 1;
	EquippedSlot.bIsEmpty = false;
	EquippedSlot.EnsureInstanceID();

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

	SyncReplicatedEquippedItems();
	if (EquippedSlot.ItemData.WeaponType != EWeaponType::None)
	{
		if (APDPlayerCharacter* PlayerCharacter = ResolvePlayerCharacter(GetOwner()))
		{
			if (UPDQuickSlotComponent* QuickSlotComponent = PlayerCharacter->GetQuickSlotComponent())
			{
				const bool bAddedQuickSlot = QuickSlotComponent->AddSlotReference(EquippedSlot);
			}
			else
			{
			}
		}
		else
		{
		}
	}
	InventoryComponent->OnInventoryChanged.Broadcast();
	BroadcastSlotChanged(TargetSlotType);
	BroadcastModificationApplied(TargetSlotType, EquippedSlot);
	return true;
}


bool UPDEquipmentComponent::TryEquipNewItem(const FPDItemData& ItemData)
{

	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		return false;
	}

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
	InitializeNewWeaponAmmoState(NewSlot);
	NewSlot.EnsureInstanceID();

	if (!ApplyCharacterEquipSideEffects(NewSlot))
	{
		return false;
	}

	FPDEquippedItem NewEquippedItem;
	NewEquippedItem.SlotType = TargetSlotType;
	NewEquippedItem.ItemSlot = NewSlot;
	EquippedItems.Add(TargetSlotType, NewEquippedItem);

	SyncReplicatedEquippedItems();
	if (ItemData.WeaponType != EWeaponType::None)
	{
		if (APDPlayerCharacter* PlayerCharacter = ResolvePlayerCharacter(GetOwner()))
		{
			if (UPDQuickSlotComponent* QuickSlotComponent = PlayerCharacter->GetQuickSlotComponent())
			{
				const bool bAddedQuickSlot = QuickSlotComponent->AddSlotReference(NewSlot);
			}
			else
			{
			}
		}
		else
		{
		}
	}
	BroadcastSlotChanged(TargetSlotType);
	BroadcastModificationApplied(TargetSlotType, NewSlot);
	return true;
}

bool UPDEquipmentComponent::UnequipItemToInventory(UPDInventoryComponent* InventoryComponent, EPDEquipmentSlotType SlotType)
{
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		ServerUnequipItemToInventory(InventoryComponent, SlotType);
		return true;
	}

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

	// ??Destroy ?ÑÏóê Î¨¥Í∏∞ ?°ÌÑ∞?êÏÑú ?∞Ì????ÅÌÉú Ï∫°Ï≤ò ???∏Î≤§?†Î¶¨Î°??åÎ†§Î≥¥ÎÇº ?¨Î°Ø??stamp.
	if (APDPlayerCharacter* PC = ResolvePlayerCharacter(GetOwner()))
	{
		if (APDRangedWeaponBase* Ranged = Cast<APDRangedWeaponBase>(FindMatchingEquippedWeapon(PC, ItemToReturn)))
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
	SyncReplicatedEquippedItems();
	BroadcastSlotChanged(SlotType);
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

	// ??Destroy ?ÑÏóê Î¨¥Í∏∞ ?°ÌÑ∞?êÏÑú ?∞Ì????ÅÌÉú Ï∫°Ï≤ò.
	if (APDPlayerCharacter* PC = ResolvePlayerCharacter(GetOwner()))
	{
		if (APDRangedWeaponBase* Ranged = Cast<APDRangedWeaponBase>(FindMatchingEquippedWeapon(PC, ItemToReturn)))
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
	InventoryComponent->Items[InventorySlotIndex].EnsureInstanceID();

	RemoveCharacterEquipSideEffects(ItemToReturn);
	EquippedItem->ItemSlot.Clear();
	SyncReplicatedEquippedItems();
	// UPDItemSoundLibrary::PlayItemMoveSound(this, ItemToReturn.ItemData); // Î©Ä??Ï∏°Ïóê ItemSoundLibrary ÎØ∏Î∞ò??	InventoryComponent->OnInventoryChanged.Broadcast();
	BroadcastSlotChanged(SlotType);
	return true;
}

bool UPDEquipmentComponent::ApplyCharacterEquipSideEffects(const FPDInventorySlot& ItemSlot) const
{
	const FPDItemData& ItemData = ItemSlot.ItemData;

	if (ItemData.WeaponType == EWeaponType::None)
	{
		return true;
	}

	APDPlayerCharacter* PlayerCharacter = ResolvePlayerCharacter(GetOwner());

	if (PlayerCharacter)
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

	APDPlayerCharacter* PlayerCharacter = ResolvePlayerCharacter(GetOwner());

	if (PlayerCharacter)
	{
		bool bRemovedWeapon = false;
		if (ItemSlot.ItemInstanceID.IsValid())
		{
			if (OutWeaponState)
			{
				bRemovedWeapon = PlayerCharacter->RemoveEquippedWeaponItemByInstanceIDPreservingState(
					ItemSlot.ItemInstanceID,
					*OutWeaponState);
			}
			else
			{
				bRemovedWeapon = PlayerCharacter->RemoveEquippedWeaponItemByInstanceID(ItemSlot.ItemInstanceID);
			}
		}

		if (!bRemovedWeapon)
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

void UPDEquipmentComponent::BroadcastAllSlotsChanged()
{
	for (const TPair<EPDEquipmentSlotType, FPDEquippedItem>& Pair : EquippedItems)
	{
		OnEquipmentSlotChanged.Broadcast(Pair.Key, Pair.Value.ItemSlot);
	}
	OnEquipmentChanged.Broadcast();
}
