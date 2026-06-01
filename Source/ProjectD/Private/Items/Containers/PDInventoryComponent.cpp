#include "Items/Containers/PDInventoryComponent.h"

#include "Core/PDPlayerComponentResolver.h"
#include "Core/PDPlayerState.h"
#include "Data/PDQuestComponent.h"
#include "Items/Data/PDItemSlotTransfer.h"
#include "Items/Equipment/PDEquipmentComponent.h"
#include "Items/Containers/PDQuickSlotComponent.h"
#include "Items/Data/PDItemSoundLibrary.h"
#include "Items/Containers/PDSecureContainerComponent.h"
#include "Weapons/Base/PDRangedWeaponBase.h"
#include "Net/UnrealNetwork.h"

namespace
{
	UPDQuestComponent* FindOwnerQuestComponent(const UActorComponent* Component)
	{
		if (!Component || !Component->GetOwner())
		{
			return nullptr;
		}

		return FPDPlayerComponentResolver::ResolveQuest(Component);
	}

	UPDEquipmentComponent* FindOwnerEquipmentComponent(const UActorComponent* Component)
	{
		if (!Component || !Component->GetOwner())
		{
			return nullptr;
		}

		return FPDPlayerComponentResolver::ResolveEquipment(Component);
	}

	int32 ResolveDefaultWeaponAmmo(const FPDItemData& ItemData)
	{
		if (ItemData.WeaponType == EWeaponType::None || !ItemData.WeaponClass)
		{
			return INDEX_NONE;
		}

		const APDRangedWeaponBase* RangedCDO = Cast<APDRangedWeaponBase>(ItemData.WeaponClass->GetDefaultObject());
		return RangedCDO ? FMath::Max(0, RangedCDO->GetMaxAmmo()) : INDEX_NONE;
	}

	void InitializeWeaponAmmoState(FPDInventorySlot& Slot)
	{
		if (Slot.WeaponState.HasPersistedAmmo())
		{
			return;
		}

		const int32 DefaultAmmo = ResolveDefaultWeaponAmmo(Slot.ItemData);
		if (DefaultAmmo != INDEX_NONE)
		{
			Slot.WeaponState.CurrentAmmo = DefaultAmmo;
		}
	}
}

UPDInventoryComponent::UPDInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UPDInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	InitializeInventory();
}

void UPDInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UPDInventoryComponent, Items, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UPDInventoryComponent, GridColumns, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UPDInventoryComponent, GridRows, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UPDInventoryComponent, Gold, COND_OwnerOnly);
}

void UPDInventoryComponent::OnRep_Items()
{
	OnInventoryChanged.Broadcast();
}

void UPDInventoryComponent::OnRep_Gold()
{
	OnInventoryChanged.Broadcast();
}

void UPDInventoryComponent::OnRep_GridConfig()
{
	OnInventoryChanged.Broadcast();
}

void UPDInventoryComponent::ServerRemoveItemFromSlot_Implementation(int32 SlotIndex, int32 Quantity)
{
	RemoveItemFromSlot(SlotIndex, Quantity);
}

void UPDInventoryComponent::ServerDropItemFromSlot_Implementation(int32 SlotIndex, int32 Quantity)
{
	DropItemFromSlot(SlotIndex, Quantity);
}

void UPDInventoryComponent::ServerMoveSlotQuantityToSlot_Implementation(int32 SourceSlotIndex, int32 TargetSlotIndex, int32 Quantity)
{
	MoveSlotQuantityToSlot(SourceSlotIndex, TargetSlotIndex, Quantity);
}

bool UPDInventoryComponent::AddItem(const FPDItemData& ItemData, int32 Quantity)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return false;
	return AddItemPartial(ItemData, Quantity) > 0;
}

bool UPDInventoryComponent::FindItemDataByID(FName ItemID, FPDItemData& OutItemData) const
{
	if (!ItemDataTable || ItemID.IsNone())
	{
		return false;
	}

	if (const FPDItemData* RowByName = ItemDataTable->FindRow<FPDItemData>(ItemID, TEXT("FindItemDataByID"), false))
	{
		if (RowByName->ItemID == ItemID || RowByName->ItemID.IsNone())
		{
			OutItemData = *RowByName;
			if (OutItemData.ItemID.IsNone())
			{
				OutItemData.ItemID = ItemID;
			}
			return true;
		}
	}

	TArray<FPDItemData*> Rows;
	ItemDataTable->GetAllRows<FPDItemData>(TEXT("FindItemDataByID"), Rows);
	for (const FPDItemData* Row : Rows)
	{
		if (Row && Row->ItemID == ItemID)
		{
			OutItemData = *Row;
			return true;
		}
	}

	return false;
}

bool UPDInventoryComponent::AddItemByID(FName ItemID, int32 Quantity)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return false;
	if (!ItemDataTable || ItemID.IsNone())
	{
		return false;
	}

	TArray<FPDItemData*> Rows;
	ItemDataTable->GetAllRows<FPDItemData>(TEXT("AddItemByID"), Rows);

	for (const FPDItemData* Row : Rows)
	{
		if (Row && Row->ItemID == ItemID)
		{
			const bool bAdded = AddItem(*Row, Quantity);
			return bAdded;
		}
	}
	return false;
}

bool UPDInventoryComponent::RemoveItem(FName ItemID, int32 Quantity)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return false;
	if (!FPDItemContainerOps::RemoveItem(Items, ItemID, Quantity))
	{
		return false;
	}

	OnInventoryChanged.Broadcast();
	return true;
}

bool UPDInventoryComponent::RemoveItemFromSlot(int32 SlotIndex, int32 Quantity)
{
	if (!GetOwner()) return false;
	if (!GetOwner()->HasAuthority())
	{
		ServerRemoveItemFromSlot(SlotIndex, Quantity);
		return true;
	}
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
	if (!GetOwner()) return false;
	if (!GetOwner()->HasAuthority())
	{
		ServerDropItemFromSlot(SlotIndex, Quantity);
		return true;
	}
	if (!Items.IsValidIndex(SlotIndex) || Quantity <= 0)
	{
		return false;
	}

	const FPDInventorySlot& Slot = Items[SlotIndex];
	if (Slot.IsEmpty())
	{
		return false;
	}

	const FName ItemID = Slot.ItemData.ItemID;
	const int32 DropAmount = FMath::Min(Quantity, Slot.Quantity);
	const bool bDropped = RemoveItemFromSlot(SlotIndex, Quantity);

	if (bDropped)
	{
		if (UPDQuestComponent* QuestComponent = FindOwnerQuestComponent(this))
		{
			QuestComponent->ReportItemDropped(ItemID, DropAmount);
		}
	}

	return bDropped;
}

bool UPDInventoryComponent::HasItem(FName ItemID, int32 Quantity) const
{
	return FPDItemContainerOps::HasItem(Items, ItemID, Quantity);
}

void UPDInventoryComponent::AddGold(int32 Amount)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	if (Amount > 0)
	{
		Gold += Amount;
		OnInventoryChanged.Broadcast();
	}
}

bool UPDInventoryComponent::SpendGold(int32 Amount)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return false;
	if (Amount <= 0 || Gold < Amount)
	{
		return false;
	}

	Gold -= Amount;
	OnInventoryChanged.Broadcast();
	return true;
}

float UPDInventoryComponent::GetCurrentWeight() const
{
	float TotalWeight = 0.f;

	for (const FPDInventorySlot& Slot : Items)
	{
		if (Slot.IsEmpty())
		{
			continue;
		}

		TotalWeight += FMath::Max(0.f, Slot.ItemData.Weight) * FMath::Max(0, Slot.Quantity);
	}

	if (const UPDSecureContainerComponent* SecureContainerComponent = GetOwner() ? GetOwner()->FindComponentByClass<UPDSecureContainerComponent>() : nullptr)
	{
		TotalWeight += SecureContainerComponent->GetCurrentWeight();
	}

	return TotalWeight;
}

float UPDInventoryComponent::GetMaxWeight() const
{
	float MaxWeight = FMath::Max(0.f, BaseCarryWeight);

	if (const UPDEquipmentComponent* EquipmentComponent = FindOwnerEquipmentComponent(this))
	{
		const FPDInventorySlot EquippedBagSlot = EquipmentComponent->GetEquippedSlot(EPDEquipmentSlotType::Bag);
		if (!EquippedBagSlot.IsEmpty())
		{
			MaxWeight += FMath::Max(0.f, EquippedBagSlot.ItemData.BagCapacityWeight);
		}
	}

	return MaxWeight;
}

bool UPDInventoryComponent::CanAddWeight(const FPDItemData& ItemData, int32 Quantity) const
{
	if (Quantity <= 0)
	{
		return true;
	}

	const float AddWeight = FMath::Max(0.f, ItemData.Weight) * Quantity;
	return GetCurrentWeight() + AddWeight <= GetMaxWeight() + KINDA_SMALL_NUMBER;
}

bool UPDInventoryComponent::CanAddSlotWeight(const FPDInventorySlot& SourceSlot, int32 Quantity) const
{
	if (SourceSlot.IsEmpty() || Quantity <= 0)
	{
		return true;
	}

	const int32 ClampedQuantity = FMath::Min(Quantity, SourceSlot.Quantity);
	return CanAddWeight(SourceSlot.ItemData, ClampedQuantity);
}

bool UPDInventoryComponent::CanFitWeightAfterEquipmentChange(const FPDInventorySlot& ItemLeavingInventory, const FPDInventorySlot& ItemEnteringInventory, float NewBagCapacityWeight) const
{
	float ExpectedWeight = GetCurrentWeight();

	if (!ItemLeavingInventory.IsEmpty())
	{
		ExpectedWeight -= FMath::Max(0.f, ItemLeavingInventory.ItemData.Weight) * FMath::Max(0, ItemLeavingInventory.Quantity);
	}

	if (!ItemEnteringInventory.IsEmpty())
	{
		ExpectedWeight += FMath::Max(0.f, ItemEnteringInventory.ItemData.Weight) * FMath::Max(0, ItemEnteringInventory.Quantity);
	}

	const float ExpectedMaxWeight = FMath::Max(0.f, BaseCarryWeight) + FMath::Max(0.f, NewBagCapacityWeight);
	return ExpectedWeight <= ExpectedMaxWeight + KINDA_SMALL_NUMBER;
}

void UPDInventoryComponent::BroadcastInventoryMessage(const FText& Message)
{
	if (!Message.IsEmpty())
	{
		OnInventoryMessage.Broadcast(Message);

		if (GetOwner() && GetOwner()->HasAuthority())
		{
			ClientBroadcastInventoryMessage(Message);
		}
	}
}

void UPDInventoryComponent::BroadcastWeightLimitExceeded()
{
	const float CurrentWeight = GetCurrentWeight();
	const float MaxWeight = GetMaxWeight();
	const FText Message = FText::FromString(TEXT("Weight limit exceeded."));

	OnInventoryWeightLimitExceeded.Broadcast(CurrentWeight, MaxWeight);
	OnInventoryMessage.Broadcast(Message);

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		ClientBroadcastWeightLimitExceeded(CurrentWeight, MaxWeight, Message);
	}
}

void UPDInventoryComponent::ClientBroadcastInventoryMessage_Implementation(const FText& Message)
{
	if (!Message.IsEmpty())
	{
		OnInventoryMessage.Broadcast(Message);
	}
}

void UPDInventoryComponent::ClientBroadcastWeightLimitExceeded_Implementation(float CurrentWeight, float MaxWeight, const FText& Message)
{
	OnInventoryWeightLimitExceeded.Broadcast(CurrentWeight, MaxWeight);

	if (!Message.IsEmpty())
	{
		OnInventoryMessage.Broadcast(Message);
	}
}

int32 UPDInventoryComponent::FindEmptySlot() const
{
	return FPDItemContainerOps::FindEmptySlot(Items);
}

int32 UPDInventoryComponent::FindSlotIndexByInstanceID(const FGuid& ItemInstanceID) const
{
	if (!ItemInstanceID.IsValid())
	{
		return INDEX_NONE;
	}

	for (int32 Index = 0; Index < Items.Num(); ++Index)
	{
		const FPDInventorySlot& Slot = Items[Index];
		if (!Slot.IsEmpty() && Slot.ItemInstanceID == ItemInstanceID)
		{
			return Index;
		}
	}

	return INDEX_NONE;
}

const FPDInventorySlot* UPDInventoryComponent::FindSlotByInstanceID(const FGuid& ItemInstanceID) const
{
	const int32 SlotIndex = FindSlotIndexByInstanceID(ItemInstanceID);
	return Items.IsValidIndex(SlotIndex) ? &Items[SlotIndex] : nullptr;
}


int32 UPDInventoryComponent::AddItemToSlotPartial(const FPDItemData& ItemData, int32 Quantity, int32 TargetSlotIndex)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return 0;
	if (Items.Num() != GetMaxSlotCount())
	{
		InitializeInventory();
	}

	if (!Items.IsValidIndex(TargetSlotIndex))
	{
		return 0;
	}

	if (!CanAddWeight(ItemData, Quantity))
	{
		BroadcastWeightLimitExceeded();
		return 0;
	}

	const int32 AddedQuantity = FPDItemSlotTransfer::AddItemToSlot(Items[TargetSlotIndex], ItemData, Quantity);
	if (AddedQuantity > 0)
	{
		InitializeWeaponAmmoState(Items[TargetSlotIndex]);
		UPDItemSoundLibrary::PlayItemMoveSound(this, ItemData);
		OnInventoryChanged.Broadcast();

		if (UPDQuestComponent* QuestComponent = FindOwnerQuestComponent(this))
		{
			QuestComponent->ReportItemAcquired(ItemData.ItemID, AddedQuantity);
			if (ItemData.bIsQuestItem)
			{
				QuestComponent->ReportQuestItemAcquired(ItemData.ItemID, AddedQuantity);
			}
		}
	}

	return AddedQuantity;
}

bool UPDInventoryComponent::MoveSlotQuantityToSlot(int32 SourceSlotIndex, int32 TargetSlotIndex, int32 Quantity)
{
	if (!GetOwner()) return false;
	if (!GetOwner()->HasAuthority())
	{
		ServerMoveSlotQuantityToSlot(SourceSlotIndex, TargetSlotIndex, Quantity);
		return true;
	}
	if (Items.Num() != GetMaxSlotCount())
	{
		InitializeInventory();
	}

	if (!Items.IsValidIndex(SourceSlotIndex) || !Items.IsValidIndex(TargetSlotIndex) || SourceSlotIndex == TargetSlotIndex || Quantity <= 0)
	{
		return false;
	}

	const FPDItemData MovedItemData = Items[SourceSlotIndex].ItemData;
	const bool bMoved = FPDItemSlotTransfer::MoveQuantity(Items[SourceSlotIndex], Items[TargetSlotIndex], Quantity);
	if (bMoved)
	{
		UPDItemSoundLibrary::PlayItemMoveSound(this, MovedItemData);
		OnInventoryChanged.Broadcast();
	}

	return bMoved;
}

int32 UPDInventoryComponent::AddItemPartial(const FPDItemData& ItemData, int32 Quantity)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return 0;
	if (ItemData.ItemID.IsNone() || Quantity <= 0)
	{
		return 0;
	}

	if (Items.Num() != GetMaxSlotCount())
	{
		InitializeInventory();
	}

	int32 RemainingQuantity = Quantity;
	int32 AddedQuantity = 0;

	auto HandleAddedItem = [&](int32 InAddedQuantity)
	{
		if (InAddedQuantity <= 0)
		{
			return;
		}

		UPDItemSoundLibrary::PlayItemMoveSound(this, ItemData);
		OnInventoryChanged.Broadcast();

		if (UPDQuestComponent* QuestComponent = FindOwnerQuestComponent(this))
		{
			QuestComponent->ReportItemAcquired(ItemData.ItemID, InAddedQuantity);
			if (ItemData.bIsQuestItem)
			{
				QuestComponent->ReportQuestItemAcquired(ItemData.ItemID, InAddedQuantity);
			}
		}
	};

	if (ItemData.ItemType == EPDItemType::Equipment || ItemData.WeaponType != EWeaponType::None)
	{
		if (UPDEquipmentComponent* EquipmentComponent = FindOwnerEquipmentComponent(this))
		{
			const bool bAutoEquipped = EquipmentComponent->TryEquipNewItem(ItemData);
			if (bAutoEquipped)
			{
				RemainingQuantity -= 1;
				AddedQuantity += 1;
			}
		}
		else
		{
		}
	}

	if (RemainingQuantity <= 0)
	{
		HandleAddedItem(AddedQuantity);
		return AddedQuantity;
	}

	if (!CanAddWeight(ItemData, RemainingQuantity))
	{
		BroadcastWeightLimitExceeded();
		HandleAddedItem(AddedQuantity);
		return AddedQuantity;
	}

	const int32 AddedToInventory = FPDItemContainerOps::AddItem(Items, ItemData, RemainingQuantity);
	RemainingQuantity -= AddedToInventory;
	AddedQuantity += AddedToInventory;

	if (AddedToInventory > 0)
	{
		UPDQuickSlotComponent* QuickSlotComponent = ItemData.WeaponType != EWeaponType::None
			? FPDPlayerComponentResolver::ResolveQuickSlot(this)
			: nullptr;

		for (FPDInventorySlot& Slot : Items)
		{
			if (!Slot.IsEmpty() && Slot.ItemData.ItemID == ItemData.ItemID)
			{
				InitializeWeaponAmmoState(Slot);
				if (QuickSlotComponent && Slot.ItemData.WeaponType != EWeaponType::None)
				{
					const bool bAddedQuickSlot = QuickSlotComponent->AddSlotReference(Slot);
				}
			}
		}
	}

	if (RemainingQuantity > 0)
	{
		BroadcastInventoryMessage(FText::FromString(TEXT("인벤토리가 가득 찼습니다.")));
	}

	if (AddedQuantity > 0)
	{
		HandleAddedItem(AddedQuantity);
	}

	return AddedQuantity;
}


int32 UPDInventoryComponent::AddSlotPartial(const FPDInventorySlot& SourceSlot)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return 0;
	if (SourceSlot.IsEmpty() || SourceSlot.Quantity <= 0)
	{
		return 0;
	}

	if (!CanAddSlotWeight(SourceSlot, SourceSlot.Quantity))
	{
		BroadcastWeightLimitExceeded();
		return 0;
	}

	if (Items.Num() != GetMaxSlotCount())
	{
		InitializeInventory();
	}

	const int32 AddedQuantity = FPDItemContainerOps::AddSlot(Items, SourceSlot);
	if (AddedQuantity <= 0)
	{
		BroadcastInventoryMessage(FText::FromString(TEXT("인벤토리가 가득 찼습니다.")));
		return 0;
	}

	for (FPDInventorySlot& Slot : Items)
	{
		if (!Slot.IsEmpty() && Slot.ItemData.ItemID == SourceSlot.ItemData.ItemID)
		{
			InitializeWeaponAmmoState(Slot);
		}
	}

	UPDItemSoundLibrary::PlayItemMoveSound(this, SourceSlot.ItemData);
	OnInventoryChanged.Broadcast();

	if (UPDQuestComponent* QuestComponent = FindOwnerQuestComponent(this))
	{
		QuestComponent->ReportItemAcquired(SourceSlot.ItemData.ItemID, AddedQuantity);
		if (SourceSlot.ItemData.bIsQuestItem)
		{
			QuestComponent->ReportQuestItemAcquired(SourceSlot.ItemData.ItemID, AddedQuantity);
		}
	}

	return AddedQuantity;
}

void UPDInventoryComponent::InitializeInventory()
{
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		return;
	}

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

	FPDItemContainerOps::EnsureInstanceIDs(Items);
	OnInventoryChanged.Broadcast();
}

void UPDInventoryComponent::ResetInventory()
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	Items.SetNum(GetMaxSlotCount());

	for (FPDInventorySlot& Slot : Items)
	{
		Slot.Clear();
	}

	OnInventoryChanged.Broadcast();
}
