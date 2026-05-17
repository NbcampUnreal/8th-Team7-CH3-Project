#include "Items/PDQuickSlotComponent.h"

#include "Items/PDInventoryComponent.h"
#include "Items/PDEquipmentComponent.h"
#include "Items/PDItemSlotTransfer.h"
#include "Items/PDStashComponent.h"

#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"

UPDQuickSlotComponent::UPDQuickSlotComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPDQuickSlotComponent::SetWeaponSlotCount(int32 NewCount)
{
	WeaponSlotCount = FMath::Clamp(NewCount, 0, GetMaxSlotCount());
	SyncQuickSlotsWithInventory();
}

void UPDQuickSlotComponent::SetSelectedIndex(int32 NewIndex)
{
	if (SelectedIndex == NewIndex)
	{
		return;
	}

	if (NewIndex != INDEX_NONE)
	{
		if (!QuickSlotItems.IsValidIndex(NewIndex) || QuickSlotItems[NewIndex].IsEmpty())
		{
			return;
		}
	}

	SelectedIndex = NewIndex;
	OnSelectionChanged.Broadcast(SelectedIndex);
}

void UPDQuickSlotComponent::BeginPlay()
{
	Super::BeginPlay();

	InitializeQuickSlots();

	if (UPDInventoryComponent* InventoryComponent = FindOwnerInventory())
	{
		InventoryComponent->OnInventoryChanged.AddUniqueDynamic(this, &UPDQuickSlotComponent::HandleInventoryChanged);
		SyncQuickSlotsWithInventory();
	}
}

void UPDQuickSlotComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CancelConsumableUse();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(WeaponQuickSlotCooldownTimerHandle);
	}

	if (UPDInventoryComponent* InventoryComponent = FindOwnerInventory())
	{
		InventoryComponent->OnInventoryChanged.RemoveDynamic(this, &UPDQuickSlotComponent::HandleInventoryChanged);
	}

	Super::EndPlay(EndPlayReason);
}

UPDInventoryComponent* UPDQuickSlotComponent::FindOwnerInventory() const
{
	if (AActor* Owner = GetOwner())
	{
		return Owner->FindComponentByClass<UPDInventoryComponent>();
	}

	return nullptr;
}

UPDEquipmentComponent* UPDQuickSlotComponent::FindOwnerEquipment() const
{
	if (AActor* Owner = GetOwner())
	{
		return Owner->FindComponentByClass<UPDEquipmentComponent>();
	}

	return nullptr;
}

int32 UPDQuickSlotComponent::GetInventoryItemQuantity(FName ItemID) const
{
	if (ItemID.IsNone())
	{
		return 0;
	}

	const UPDInventoryComponent* InventoryComponent = FindOwnerInventory();
	if (!InventoryComponent)
	{
		return 0;
	}

	int32 Quantity = 0;
	for (const FPDInventorySlot& Slot : InventoryComponent->Items)
	{
		if (!Slot.IsEmpty() && Slot.ItemData.ItemID == ItemID)
		{
			Quantity += Slot.Quantity;
		}
	}

	return Quantity;
}

int32 UPDQuickSlotComponent::GetAvailableItemQuantity(const FPDItemData& ItemData) const
{
	int32 Quantity = GetInventoryItemQuantity(ItemData.ItemID);
	if (ItemData.ItemType == EPDItemType::Equipment && IsEquippedItem(ItemData))
	{
		++Quantity;
	}
	return Quantity;
}

int32 UPDQuickSlotComponent::FindInventorySlotByItemID(FName ItemID) const
{
	if (ItemID.IsNone())
	{
		return INDEX_NONE;
	}

	const UPDInventoryComponent* InventoryComponent = FindOwnerInventory();
	if (!InventoryComponent)
	{
		return INDEX_NONE;
	}

	for (int32 Index = 0; Index < InventoryComponent->Items.Num(); ++Index)
	{
		const FPDInventorySlot& Slot = InventoryComponent->Items[Index];
		if (!Slot.IsEmpty() && Slot.ItemData.ItemID == ItemID)
		{
			return Index;
		}
	}

	return INDEX_NONE;
}

int32 UPDQuickSlotComponent::FindWeaponQuickSlotByItemID(FName ItemID) const
{
	if (ItemID.IsNone())
	{
		return INDEX_NONE;
	}

	for (int32 Index = 0; Index < QuickSlotItems.Num(); ++Index)
	{
		if (!IsWeaponQuickSlot(Index))
		{
			continue;
		}

		const FPDInventorySlot& Slot = QuickSlotItems[Index];
		if (!Slot.IsEmpty() && Slot.ItemData.ItemID == ItemID)
		{
			return Index;
		}
	}

	return INDEX_NONE;
}

bool UPDQuickSlotComponent::IsEquippedItem(const FPDItemData& ItemData) const
{
	const UPDEquipmentComponent* EquipmentComponent = FindOwnerEquipment();
	if (!EquipmentComponent || ItemData.ItemID.IsNone())
	{
		return false;
	}

	const FPDInventorySlot EquippedWeapon = EquipmentComponent->GetEquippedSlot(EPDEquipmentSlotType::Weapon);
	return !EquippedWeapon.IsEmpty() && EquippedWeapon.ItemData.ItemID == ItemData.ItemID;
}

bool UPDQuickSlotComponent::SyncQuickSlotsWithInventory()
{
	bool bChanged = false;

	for (int32 Index = 0; Index < QuickSlotItems.Num(); ++Index)
	{
		FPDInventorySlot& Slot = QuickSlotItems[Index];
		if (Slot.IsEmpty())
		{
			continue;
		}

		if (!CanAssignItemToSlot(Slot.ItemData, Index))
		{
			Slot.Clear();
			bChanged = true;
			continue;
		}

		const int32 AvailableQuantity = GetAvailableItemQuantity(Slot.ItemData);
		if (AvailableQuantity <= 0)
		{
			Slot.Clear();
			bChanged = true;
			continue;
		}

		const int32 NewQuantity = Slot.ItemData.ItemType == EPDItemType::Equipment ? 1 : AvailableQuantity;
		if (Slot.Quantity != NewQuantity || Slot.bIsEmpty)
		{
			Slot.Quantity = NewQuantity;
			Slot.bIsEmpty = false;
			bChanged = true;
		}
	}

	return bChanged;
}

void UPDQuickSlotComponent::HandleInventoryChanged()
{
	if (SyncQuickSlotsWithInventory())
	{
		OnQuickSlotsChanged.Broadcast();
	}
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

bool UPDQuickSlotComponent::IsWeaponQuickSlot(int32 SlotIndex) const
{
	return SlotIndex >= 0 && SlotIndex < WeaponSlotCount;
}

bool UPDQuickSlotComponent::IsConsumableQuickSlot(int32 SlotIndex) const
{
	return SlotIndex >= WeaponSlotCount && SlotIndex < GetMaxSlotCount();
}

bool UPDQuickSlotComponent::CanAssignItemToSlot(const FPDItemData& ItemData, int32 SlotIndex) const
{
	if (ItemData.ItemID.IsNone() || SlotIndex < 0 || SlotIndex >= GetMaxSlotCount())
	{
		return false;
	}

	if (IsWeaponQuickSlot(SlotIndex))
	{
		return ItemData.ItemType == EPDItemType::Equipment && ItemData.EquipmentSlotType == EPDEquipmentSlotType::Weapon;
	}

	return IsConsumableQuickSlot(SlotIndex) && ItemData.ItemType == EPDItemType::Consumable;
}

int32 UPDQuickSlotComponent::FindEmptySlotForItem(const FPDItemData& ItemData) const
{
	for (int32 Index = 0; Index < QuickSlotItems.Num(); ++Index)
	{
		if (QuickSlotItems[Index].IsEmpty() && CanAssignItemToSlot(ItemData, Index))
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
	if (QuickSlotItems.Num() != GetMaxSlotCount())
	{
		InitializeQuickSlots();
	}

	const int32 TargetSlotIndex = FindEmptySlotForItem(ItemData);
	return AddItemToSlotPartial(ItemData, Quantity, TargetSlotIndex);
}

int32 UPDQuickSlotComponent::AddItemToSlotPartial(const FPDItemData& ItemData, int32 Quantity, int32 TargetSlotIndex)
{
	if (ItemData.ItemID.IsNone() || Quantity <= 0)
	{
		return 0;
	}

	if (QuickSlotItems.Num() != GetMaxSlotCount())
	{
		InitializeQuickSlots();
	}

	if (!QuickSlotItems.IsValidIndex(TargetSlotIndex) || !CanAssignItemToSlot(ItemData, TargetSlotIndex))
	{
		return 0;
	}

	FPDInventorySlot& TargetSlot = QuickSlotItems[TargetSlotIndex];
	if (!TargetSlot.IsEmpty() && TargetSlot.ItemData.ItemID != ItemData.ItemID)
	{
		return 0;
	}

	if (ItemData.ItemType == EPDItemType::Equipment)
	{
		TargetSlot.ItemData = ItemData;
		TargetSlot.Quantity = 1;
		TargetSlot.bIsEmpty = false;
		OnQuickSlotsChanged.Broadcast();
		return 1;
	}

	const int32 AddedQuantity = FPDItemSlotTransfer::AddItemToSlot(TargetSlot, ItemData, Quantity);
	if (AddedQuantity > 0)
	{
		SyncQuickSlotsWithInventory();
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

	if (!QuickSlotItems.IsValidIndex(SourceSlotIndex) || !QuickSlotItems.IsValidIndex(TargetSlotIndex) || SourceSlotIndex == TargetSlotIndex)
	{
		return false;
	}

	const FPDInventorySlot SourceSlot = QuickSlotItems[SourceSlotIndex];
	const FPDInventorySlot TargetSlot = QuickSlotItems[TargetSlotIndex];
	if (SourceSlot.IsEmpty())
	{
		return false;
	}

	if (!CanAssignItemToSlot(SourceSlot.ItemData, TargetSlotIndex))
	{
		return false;
	}

	if (!TargetSlot.IsEmpty() && !CanAssignItemToSlot(TargetSlot.ItemData, SourceSlotIndex))
	{
		return false;
	}

	Swap(QuickSlotItems[SourceSlotIndex], QuickSlotItems[TargetSlotIndex]);
	SyncQuickSlotsWithInventory();
	OnQuickSlotsChanged.Broadcast();
	return true;
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

	if (!SourceInventory || !SourceInventory->Items.IsValidIndex(SourceSlotIndex) || !QuickSlotItems.IsValidIndex(TargetQuickSlotIndex))
	{
		return false;
	}

	const FPDInventorySlot& SourceSlot = SourceInventory->Items[SourceSlotIndex];
	if (SourceSlot.IsEmpty() || !CanAssignItemToSlot(SourceSlot.ItemData, TargetQuickSlotIndex))
	{
		return false;
	}

	for (int32 Index = 0; Index < QuickSlotItems.Num(); ++Index)
	{
		if (Index != TargetQuickSlotIndex && !QuickSlotItems[Index].IsEmpty() && QuickSlotItems[Index].ItemData.ItemID == SourceSlot.ItemData.ItemID)
		{
			QuickSlotItems[Index].Clear();
		}
	}

	FPDInventorySlot& TargetSlot = QuickSlotItems[TargetQuickSlotIndex];
	TargetSlot.ItemData = SourceSlot.ItemData;
	TargetSlot.Quantity = SourceSlot.ItemData.ItemType == EPDItemType::Equipment ? 1 : FMath::Max(1, GetInventoryItemQuantity(SourceSlot.ItemData.ItemID));
	TargetSlot.bIsEmpty = false;

	SyncQuickSlotsWithInventory();
	OnQuickSlotsChanged.Broadcast();
	return true;
}

bool UPDQuickSlotComponent::StoreStashSlotQuantityToSlot(UPDStashComponent* SourceStash, int32 SourceStashSlotIndex, int32 TargetQuickSlotIndex, int32 Quantity)
{
	return false;
}

bool UPDQuickSlotComponent::TakeQuickSlotQuantityToInventorySlot(UPDInventoryComponent* TargetInventory, int32 QuickSlotIndex, int32 TargetInventorySlotIndex, int32 Quantity)
{
	return RemoveItemFromSlot(QuickSlotIndex, Quantity);
}

bool UPDQuickSlotComponent::TakeQuickSlotQuantityToStashSlot(UPDStashComponent* TargetStash, int32 QuickSlotIndex, int32 TargetStashSlotIndex, int32 Quantity)
{
	return RemoveItemFromSlot(QuickSlotIndex, Quantity);
}

bool UPDQuickSlotComponent::RemoveItemFromSlot(int32 SlotIndex, int32 Quantity)
{
	if (!QuickSlotItems.IsValidIndex(SlotIndex))
	{
		return false;
	}

	if (QuickSlotItems[SlotIndex].IsEmpty())
	{
		return false;
	}

	QuickSlotItems[SlotIndex].Clear();
	OnQuickSlotsChanged.Broadcast();
	return true;
}

bool UPDQuickSlotComponent::UseInventoryConsumableSlot(int32 InventorySlotIndex)
{
	if (bIsUsingConsumable)
	{
		return false;
	}

	const UPDInventoryComponent* InventoryComponent = FindOwnerInventory();
	if (!InventoryComponent || !InventoryComponent->Items.IsValidIndex(InventorySlotIndex))
	{
		return false;
	}

	const FPDInventorySlot Slot = InventoryComponent->Items[InventorySlotIndex];
	if (Slot.IsEmpty() || Slot.ItemData.ItemType != EPDItemType::Consumable)
	{
		return false;
	}

	return BeginConsumableUse(INDEX_NONE, Slot);
}

bool UPDQuickSlotComponent::EquipInventoryWeaponSlot(int32 InventorySlotIndex)
{
	const UPDInventoryComponent* InventoryComponent = FindOwnerInventory();
	if (!InventoryComponent || !InventoryComponent->Items.IsValidIndex(InventorySlotIndex))
	{
		return false;
	}

	const FPDInventorySlot Slot = InventoryComponent->Items[InventorySlotIndex];
	if (Slot.IsEmpty() || Slot.ItemData.ItemType != EPDItemType::Equipment || Slot.ItemData.EquipmentSlotType != EPDEquipmentSlotType::Weapon)
	{
		return false;
	}

	const int32 CooldownSlotIndex = FindWeaponQuickSlotByItemID(Slot.ItemData.ItemID);
	return EquipWeaponFromInventorySlot(InventorySlotIndex, CooldownSlotIndex);
}

bool UPDQuickSlotComponent::UseQuickSlot(int32 SlotIndex)
{
	if (QuickSlotItems.Num() != GetMaxSlotCount())
	{
		InitializeQuickSlots();
	}

	if (!QuickSlotItems.IsValidIndex(SlotIndex))
	{
		return false;
	}

	const FPDInventorySlot Slot = QuickSlotItems[SlotIndex];
	if (Slot.IsEmpty() || !CanAssignItemToSlot(Slot.ItemData, SlotIndex))
	{
		return false;
	}

	if (IsWeaponQuickSlot(SlotIndex))
	{
		return UseWeaponQuickSlot(SlotIndex, Slot);
	}

	if (IsConsumableQuickSlot(SlotIndex))
	{
		if (bIsUsingConsumable && UsingConsumableSlotIndex == SlotIndex)
		{
			return CancelConsumableUse();
		}

		if (bIsUsingConsumable)
		{
			return false;
		}

		return BeginConsumableUse(SlotIndex, Slot);
	}

	return false;
}

bool UPDQuickSlotComponent::UseWeaponQuickSlot(int32 SlotIndex, const FPDInventorySlot& Slot)
{
	if (Slot.IsEmpty() || !IsWeaponQuickSlot(SlotIndex))
	{
		return false;
	}

	const int32 InventorySlotIndex = FindInventorySlotByItemID(Slot.ItemData.ItemID);
	if (InventorySlotIndex == INDEX_NONE)
	{
		SyncQuickSlotsWithInventory();
		OnQuickSlotsChanged.Broadcast();
		return false;
	}

	return EquipWeaponFromInventorySlot(InventorySlotIndex, SlotIndex);
}

bool UPDQuickSlotComponent::EquipWeaponFromInventorySlot(int32 InventorySlotIndex, int32 CooldownSlotIndex)
{
	if (bIsUsingConsumable || IsWeaponQuickSlotOnCooldown())
	{
		return false;
	}

	UPDEquipmentComponent* EquipmentComponent = FindOwnerEquipment();
	UPDInventoryComponent* InventoryComponent = FindOwnerInventory();
	if (!EquipmentComponent || !InventoryComponent || !InventoryComponent->Items.IsValidIndex(InventorySlotIndex))
	{
		return false;
	}

	const FPDInventorySlot InventorySlot = InventoryComponent->Items[InventorySlotIndex];
	if (InventorySlot.IsEmpty() || InventorySlot.ItemData.ItemType != EPDItemType::Equipment || InventorySlot.ItemData.EquipmentSlotType != EPDEquipmentSlotType::Weapon)
	{
		return false;
	}

	if (IsEquippedItem(InventorySlot.ItemData))
	{
		return false;
	}

	const bool bEquipped = EquipmentComponent->EquipItemFromInventoryToSlot(InventoryComponent, InventorySlotIndex, EPDEquipmentSlotType::Weapon);
	if (!bEquipped)
	{
		return false;
	}

	if (QuickSlotItems.IsValidIndex(CooldownSlotIndex))
	{
		SetSelectedIndex(CooldownSlotIndex);
	}

	SyncQuickSlotsWithInventory();
	OnQuickSlotsChanged.Broadcast();
	StartWeaponQuickSlotCooldown(CooldownSlotIndex);
	return true;
}

bool UPDQuickSlotComponent::BeginConsumableUse(int32 SlotIndex, const FPDInventorySlot& Slot)
{
	UPDInventoryComponent* InventoryComponent = FindOwnerInventory();
	if (!InventoryComponent || !InventoryComponent->HasItem(Slot.ItemData.ItemID, 1))
	{
		SyncQuickSlotsWithInventory();
		OnQuickSlotsChanged.Broadcast();
		return false;
	}

	const float Duration = FMath::Max(0.f, Slot.ItemData.UseDuration);
	if (Duration <= 0.f)
	{
		if (ConsumeItem(Slot))
		{
			if (QuickSlotItems.IsValidIndex(SlotIndex))
			{
				SetSelectedIndex(SlotIndex);
			}
			OnConsumableUseCompleted.Broadcast(SlotIndex, Slot.ItemData);
			return true;
		}
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	bIsUsingConsumable = true;
	UsingConsumableSlotIndex = SlotIndex;
	PendingConsumableSlot = Slot;
	ConsumableUseStartTime = World->GetTimeSeconds();
	ConsumableUseEndTime = ConsumableUseStartTime + Duration;

	ApplyConsumableMoveSpeed();
	World->GetTimerManager().SetTimer(ConsumableUseTimerHandle, this, &UPDQuickSlotComponent::FinishConsumableUse, Duration, false);
	if (QuickSlotItems.IsValidIndex(SlotIndex))
	{
		SetSelectedIndex(SlotIndex);
	}
	OnConsumableUseStarted.Broadcast(SlotIndex, Slot.ItemData, Duration);
	return true;
}

void UPDQuickSlotComponent::FinishConsumableUse()
{
	if (!bIsUsingConsumable)
	{
		return;
	}

	const int32 CompletedSlotIndex = UsingConsumableSlotIndex;
	const FPDInventorySlot CompletedSlot = PendingConsumableSlot;

	bIsUsingConsumable = false;
	UsingConsumableSlotIndex = INDEX_NONE;
	ConsumableUseStartTime = 0.f;
	ConsumableUseEndTime = 0.f;
	PendingConsumableSlot.Clear();
	RestoreConsumableMoveSpeed();

	if (ConsumeItem(CompletedSlot))
	{
		if (QuickSlotItems.IsValidIndex(CompletedSlotIndex))
		{
			SetSelectedIndex(CompletedSlotIndex);
		}
		OnConsumableUseCompleted.Broadcast(CompletedSlotIndex, CompletedSlot.ItemData);
		return;
	}

	SyncQuickSlotsWithInventory();
	OnQuickSlotsChanged.Broadcast();
}

bool UPDQuickSlotComponent::ConsumeItem(const FPDInventorySlot& Slot)
{
	UPDInventoryComponent* InventoryComponent = FindOwnerInventory();
	if (!InventoryComponent || Slot.IsEmpty() || !InventoryComponent->HasItem(Slot.ItemData.ItemID, 1))
	{
		return false;
	}

	if (!InventoryComponent->RemoveItem(Slot.ItemData.ItemID, 1))
	{
		return false;
	}

	if (Slot.ItemData.UseEffect)
	{
		UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
		if (ASC)
		{
			FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
			Context.AddSourceObject(GetOwner());
			ASC->ApplyGameplayEffectToSelf(Slot.ItemData.UseEffect->GetDefaultObject<UGameplayEffect>(), 1.f, Context);
		}
	}

	SyncQuickSlotsWithInventory();
	OnQuickSlotsChanged.Broadcast();
	return true;
}

bool UPDQuickSlotComponent::CancelConsumableUse()
{
	if (!bIsUsingConsumable)
	{
		return false;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ConsumableUseTimerHandle);
	}

	const int32 CanceledSlotIndex = UsingConsumableSlotIndex;
	const FPDItemData CanceledItemData = PendingConsumableSlot.ItemData;

	bIsUsingConsumable = false;
	UsingConsumableSlotIndex = INDEX_NONE;
	PendingConsumableSlot.Clear();
	ConsumableUseStartTime = 0.f;
	ConsumableUseEndTime = 0.f;
	RestoreConsumableMoveSpeed();
	OnConsumableUseCanceled.Broadcast(CanceledSlotIndex, CanceledItemData);
	return true;
}

float UPDQuickSlotComponent::GetConsumableUseRemainingTime() const
{
	if (!bIsUsingConsumable)
	{
		return 0.f;
	}

	const UWorld* World = GetWorld();
	return World ? FMath::Max(0.f, ConsumableUseEndTime - World->GetTimeSeconds()) : 0.f;
}

float UPDQuickSlotComponent::GetConsumableUseProgress() const
{
	if (!bIsUsingConsumable || ConsumableUseEndTime <= ConsumableUseStartTime)
	{
		return 0.f;
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return 0.f;
	}

	const float Elapsed = World->GetTimeSeconds() - ConsumableUseStartTime;
	const float Duration = ConsumableUseEndTime - ConsumableUseStartTime;
	return FMath::Clamp(Elapsed / Duration, 0.f, 1.f);
}

bool UPDQuickSlotComponent::IsWeaponQuickSlotOnCooldown() const
{
	return bWeaponQuickSlotCooldownActive && GetWeaponQuickSlotCooldownRemainingTime() > 0.f;
}

float UPDQuickSlotComponent::GetWeaponQuickSlotCooldownRemainingTime() const
{
	if (!bWeaponQuickSlotCooldownActive)
	{
		return 0.f;
	}

	const UWorld* World = GetWorld();
	return World ? FMath::Max(0.f, WeaponCooldownEndTime - World->GetTimeSeconds()) : 0.f;
}

void UPDQuickSlotComponent::ApplyConsumableMoveSpeed()
{
	UCharacterMovementComponent* MovementComponent = FindOwnerMovementComponent();
	if (!MovementComponent || bMoveSpeedAdjusted)
	{
		return;
	}

	CachedMaxWalkSpeed = MovementComponent->MaxWalkSpeed;
	MovementComponent->MaxWalkSpeed = CachedMaxWalkSpeed * ConsumableMoveSpeedMultiplier;
	bMoveSpeedAdjusted = true;
}

void UPDQuickSlotComponent::RestoreConsumableMoveSpeed()
{
	UCharacterMovementComponent* MovementComponent = FindOwnerMovementComponent();
	if (!MovementComponent || !bMoveSpeedAdjusted)
	{
		bMoveSpeedAdjusted = false;
		CachedMaxWalkSpeed = 0.f;
		return;
	}

	MovementComponent->MaxWalkSpeed = CachedMaxWalkSpeed;
	bMoveSpeedAdjusted = false;
	CachedMaxWalkSpeed = 0.f;
}

UCharacterMovementComponent* UPDQuickSlotComponent::FindOwnerMovementComponent() const
{
	const ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	return OwnerCharacter ? OwnerCharacter->GetCharacterMovement() : nullptr;
}

void UPDQuickSlotComponent::StartWeaponQuickSlotCooldown(int32 SlotIndex)
{
	UWorld* World = GetWorld();
	if (!World || WeaponSwitchCooldown <= 0.f)
	{
		return;
	}

	bWeaponQuickSlotCooldownActive = true;
	WeaponCooldownSlotIndex = SlotIndex;
	WeaponCooldownEndTime = World->GetTimeSeconds() + WeaponSwitchCooldown;
	World->GetTimerManager().SetTimer(WeaponQuickSlotCooldownTimerHandle, this, &UPDQuickSlotComponent::FinishWeaponQuickSlotCooldown, WeaponSwitchCooldown, false);
	OnWeaponQuickSlotCooldownStarted.Broadcast(SlotIndex, WeaponSwitchCooldown, WeaponCooldownEndTime);
}

void UPDQuickSlotComponent::FinishWeaponQuickSlotCooldown()
{
	const int32 FinishedSlotIndex = WeaponCooldownSlotIndex;
	bWeaponQuickSlotCooldownActive = false;
	WeaponCooldownSlotIndex = INDEX_NONE;
	WeaponCooldownEndTime = 0.f;
	OnWeaponQuickSlotCooldownFinished.Broadcast(FinishedSlotIndex);
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
