#include "Items/Containers/PDQuickSlotComponent.h"

#include "Characters/PDPlayerCharacter.h"
#include "Core/PDPlayerComponentResolver.h"
#include "Core/PDPlayerController.h"
#include "Core/PDPlayerState.h"
#include "Core/PDPlayerUIManagerComponent.h"
#include "Items/Containers/PDInventoryComponent.h"
#include "Items/Equipment/PDEquipmentComponent.h"
#include "Items/Data/PDItemSoundLibrary.h"
#include "Ability/GA_UseConsumableAbility.h"
#include "Weapons/Base/PDWeaponBase.h"
#include "Components/AudioComponent.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameplayTag/PDGameplayTags.h"
#include "TimerManager.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Net/UnrealNetwork.h"

namespace
{
	void PDRefreshOwningQuickSlotUI(const UPDQuickSlotComponent* QuickSlotComponent)
	{
		const APDPlayerState* OwnerPlayerState = QuickSlotComponent ? Cast<APDPlayerState>(QuickSlotComponent->GetOwner()) : nullptr;
		UWorld* World = QuickSlotComponent ? QuickSlotComponent->GetWorld() : nullptr;
		if (!OwnerPlayerState || !World)
		{
			return;
		}

		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			APDPlayerController* PlayerController = Cast<APDPlayerController>(It->Get());
			if (!PlayerController || !PlayerController->IsLocalController() || PlayerController->GetPDPlayerState() != OwnerPlayerState)
			{
				continue;
			}

			if (UPDPlayerUIManagerComponent* UIManager = PlayerController->GetUIManagerComponent())
			{
				UIManager->RefreshNewQuickSlots();
			}
			return;
		}
	}
}

UPDQuickSlotComponent::UPDQuickSlotComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UPDQuickSlotComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UPDQuickSlotComponent, QuickSlotItems);
	DOREPLIFETIME(UPDQuickSlotComponent, SelectedIndex);
	DOREPLIFETIME(UPDQuickSlotComponent, bIsUsingConsumable);
	DOREPLIFETIME(UPDQuickSlotComponent, UsingConsumableSlotIndex);
	DOREPLIFETIME(UPDQuickSlotComponent, PendingConsumableItemID);
	DOREPLIFETIME(UPDQuickSlotComponent, PendingConsumableInstanceID);
	DOREPLIFETIME(UPDQuickSlotComponent, ConsumableUseStartTime);
	DOREPLIFETIME(UPDQuickSlotComponent, ConsumableUseEndTime);
	DOREPLIFETIME(UPDQuickSlotComponent, bLastConsumableUseCompleted);
	DOREPLIFETIME(UPDQuickSlotComponent, bWeaponQuickSlotCooldownActive);
	DOREPLIFETIME(UPDQuickSlotComponent, WeaponCooldownSlotIndex);
	DOREPLIFETIME(UPDQuickSlotComponent, WeaponCooldownEndTime);
}

void UPDQuickSlotComponent::OnRep_QuickSlotItems()
{

	for (int32 Index = 0; Index < QuickSlotItems.Num(); ++Index)
	{
	}

	if (UPDInventoryComponent* InventoryComponent = FindOwnerInventory())
	{
		InventoryComponent->OnInventoryChanged.AddUniqueDynamic(this, &UPDQuickSlotComponent::HandleInventoryChanged);
	}

	if (UPDEquipmentComponent* EquipmentComponent = FindOwnerEquipment())
	{
		EquipmentComponent->OnEquipmentChanged.AddUniqueDynamic(this, &UPDQuickSlotComponent::HandleEquipmentChanged);
	}

	OnQuickSlotsChanged.Broadcast();
	PDRefreshOwningQuickSlotUI(this);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			OnQuickSlotsChanged.Broadcast();
			PDRefreshOwningQuickSlotUI(this);
		}));
	}
}

void UPDQuickSlotComponent::OnRep_SelectedIndex()
{
	OnSelectionChanged.Broadcast(SelectedIndex);
}

void UPDQuickSlotComponent::OnRep_WeaponCooldownState()
{
	UWorld* World = GetWorld();

	if (bWeaponQuickSlotCooldownActive)
	{
		const float CooldownDuration = FMath::Max(0.f, WeaponSwitchCooldown);
		LocalWeaponCooldownEndTime = World ? World->GetTimeSeconds() + CooldownDuration : 0.f;
		bWeaponCooldownPresentationActive = true;
		PresentedWeaponCooldownSlotIndex = WeaponCooldownSlotIndex;

		OnWeaponQuickSlotCooldownStarted.Broadcast(WeaponCooldownSlotIndex, CooldownDuration, LocalWeaponCooldownEndTime);
		OnQuickSlotsChanged.Broadcast();
		PDRefreshOwningQuickSlotUI(this);
		return;
	}

	LocalWeaponCooldownEndTime = 0.f;

	if (bWeaponCooldownPresentationActive)
	{
		const int32 FinishedSlotIndex = PresentedWeaponCooldownSlotIndex;
		bWeaponCooldownPresentationActive = false;
		PresentedWeaponCooldownSlotIndex = INDEX_NONE;
		OnWeaponQuickSlotCooldownFinished.Broadcast(FinishedSlotIndex);
	}

	OnQuickSlotsChanged.Broadcast();
	PDRefreshOwningQuickSlotUI(this);
}

void UPDQuickSlotComponent::ForceQuickSlotReplication() const
{
	if (AActor* OwnerActor = GetOwner(); OwnerActor && OwnerActor->HasAuthority())
	{
		OwnerActor->ForceNetUpdate();
	}
}

void UPDQuickSlotComponent::OnRep_ConsumableUseState()
{
	if (bIsUsingConsumable)
	{
		const FPDInventorySlot PendingSlot = ResolvePendingConsumableSlot();
		if (!bConsumableUsePresentationActive && !PendingSlot.IsEmpty())
		{
			bConsumableUsePresentationActive = true;
			PresentedConsumableSlotIndex = UsingConsumableSlotIndex;
			PresentedConsumableItemData = PendingSlot.ItemData;
			StartConsumableUseSound(PendingSlot.ItemData);
			OnConsumableUseStarted.Broadcast(
				UsingConsumableSlotIndex,
				PendingSlot.ItemData,
				FMath::Max(0.f, ConsumableUseEndTime - ConsumableUseStartTime));
		}
		return;
	}

	if (bConsumableUsePresentationActive)
	{
		const int32 FinishedSlotIndex = PresentedConsumableSlotIndex;
		const FPDItemData FinishedItemData = PresentedConsumableItemData;
		bConsumableUsePresentationActive = false;
		PresentedConsumableSlotIndex = INDEX_NONE;
		PresentedConsumableItemData = FPDItemData();
		StopConsumableUseSound();
		if (bLastConsumableUseCompleted)
		{
			OnConsumableUseCompleted.Broadcast(FinishedSlotIndex, FinishedItemData);
		}
		else
		{
			OnConsumableUseCanceled.Broadcast(FinishedSlotIndex, FinishedItemData);
		}
	}
	else
	{
		StopConsumableUseSound();
	}
}

void UPDQuickSlotComponent::ServerSetSelectedIndex_Implementation(int32 NewIndex)
{
	SetSelectedIndex(NewIndex);
}

void UPDQuickSlotComponent::ServerResetQuickSlots_Implementation()
{
	ResetQuickSlots();
}

void UPDQuickSlotComponent::ServerMoveSlotQuantityToSlot_Implementation(int32 SourceSlotIndex, int32 TargetSlotIndex, int32 Quantity)
{
	MoveSlotQuantityToSlot(SourceSlotIndex, TargetSlotIndex, Quantity);
}

void UPDQuickSlotComponent::ServerStoreInventorySlotQuantityToSlot_Implementation(UPDInventoryComponent* SourceInventory, int32 SourceSlotIndex, int32 TargetQuickSlotIndex, int32 Quantity)
{
	StoreInventorySlotQuantityToSlot(SourceInventory, SourceSlotIndex, TargetQuickSlotIndex, Quantity);
}

void UPDQuickSlotComponent::ServerRemoveItemFromSlot_Implementation(int32 SlotIndex, int32 Quantity)
{
	RemoveItemFromSlot(SlotIndex, Quantity);
}

void UPDQuickSlotComponent::ServerUseQuickSlot_Implementation(int32 SlotIndex)
{
	UseQuickSlot(SlotIndex);
}

void UPDQuickSlotComponent::ServerUseInventoryConsumableSlot_Implementation(int32 InventorySlotIndex)
{
	UseInventoryConsumableSlot(InventorySlotIndex);
}

void UPDQuickSlotComponent::ServerEquipInventoryWeaponSlot_Implementation(int32 InventorySlotIndex)
{
	EquipInventoryWeaponSlot(InventorySlotIndex);
}

void UPDQuickSlotComponent::ServerCancelConsumableUse_Implementation()
{
	CancelConsumableUse();
}

void UPDQuickSlotComponent::SetWeaponSlotCount(int32 NewCount)
{
	WeaponSlotCount = FMath::Clamp(NewCount, 0, GetMaxSlotCount());
	SyncQuickSlotsWithInventory();
}

void UPDQuickSlotComponent::SetSelectedIndex(int32 NewIndex)
{
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		ServerSetSelectedIndex(NewIndex);
		return;
	}

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

	if (UPDEquipmentComponent* EquipmentComponent = FindOwnerEquipment())
	{
		EquipmentComponent->OnEquipmentChanged.AddUniqueDynamic(this, &UPDQuickSlotComponent::HandleEquipmentChanged);
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

	if (UPDEquipmentComponent* EquipmentComponent = FindOwnerEquipment())
	{
		EquipmentComponent->OnEquipmentChanged.RemoveDynamic(this, &UPDQuickSlotComponent::HandleEquipmentChanged);
	}

	Super::EndPlay(EndPlayReason);
}

UPDInventoryComponent* UPDQuickSlotComponent::FindOwnerInventory() const
{
	return FPDPlayerComponentResolver::ResolveInventory(this);
}

UPDEquipmentComponent* UPDQuickSlotComponent::FindOwnerEquipment() const
{
	return FPDPlayerComponentResolver::ResolveEquipment(this);
}

APDPlayerCharacter* UPDQuickSlotComponent::FindOwnerPlayerCharacter() const
{
	if (APDPlayerCharacter* PlayerCharacter = Cast<APDPlayerCharacter>(GetOwner()))
	{
		return PlayerCharacter;
	}

	if (const APDPlayerState* PDPlayerState = Cast<APDPlayerState>(GetOwner()))
	{
		return PDPlayerState->GetPDPlayerCharacter();
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
	if ((ItemData.ItemType == EPDItemType::Equipment || ItemData.WeaponType != EWeaponType::None) && IsEquippedItem(ItemData))
	{
		++Quantity;
	}
	return Quantity;
}

int32 UPDQuickSlotComponent::FindInventorySlotByInstanceID(const FGuid& ItemInstanceID) const
{
	const UPDInventoryComponent* InventoryComponent = FindOwnerInventory();
	return InventoryComponent ? InventoryComponent->FindSlotIndexByInstanceID(ItemInstanceID) : INDEX_NONE;
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

int32 UPDQuickSlotComponent::FindWeaponQuickSlotByInstanceID(const FGuid& ItemInstanceID) const
{
	if (!ItemInstanceID.IsValid())
	{
		return INDEX_NONE;
	}

	for (int32 Index = 0; Index < QuickSlotItems.Num(); ++Index)
	{
		if (!IsWeaponQuickSlot(Index))
		{
			continue;
		}

		const FPDQuickSlotReference& Slot = QuickSlotItems[Index];
		if (!Slot.IsEmpty() && Slot.ItemInstanceID == ItemInstanceID)
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

		const FPDQuickSlotReference& Slot = QuickSlotItems[Index];
		if (!Slot.IsEmpty() && Slot.ItemID == ItemID)
		{
			return Index;
		}
	}

	return INDEX_NONE;
}

bool UPDQuickSlotComponent::IsEquippedItem(const FPDItemData& ItemData) const
{
	if (ItemData.WeaponType != EWeaponType::None)
	{
		if (const APDPlayerCharacter* PlayerCharacter = FindOwnerPlayerCharacter())
		{
			const EWeaponSlot WeaponSlot = PlayerCharacter->GetSlotForWeaponType(ItemData.WeaponType);
			const APDWeaponBase* Weapon = PlayerCharacter->GetWeaponInSlot(WeaponSlot);
			if (Weapon && Weapon->GetItemID() == ItemData.ItemID)
			{
				return true;
			}
		}
	}

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


	if (QuickSlotItems.Num() == GetMaxSlotCount())
	{
		if (const UPDEquipmentComponent* EquipmentComponent = FindOwnerEquipment())
		{
			const FPDInventorySlot EquippedWeapon = EquipmentComponent->GetEquippedSlot(EPDEquipmentSlotType::Weapon);
			if (!EquippedWeapon.IsEmpty() && EquippedWeapon.ItemData.WeaponType != EWeaponType::None)
			{
				int32 ExistingIndex = EquippedWeapon.ItemInstanceID.IsValid()
					? FindWeaponQuickSlotByInstanceID(EquippedWeapon.ItemInstanceID)
					: INDEX_NONE;
				if (ExistingIndex == INDEX_NONE)
				{
					ExistingIndex = FindWeaponQuickSlotByItemID(EquippedWeapon.ItemData.ItemID);
				}
				if (ExistingIndex == INDEX_NONE)
				{
					const int32 TargetSlotIndex = FindEmptySlotForItem(EquippedWeapon.ItemData);
					if (QuickSlotItems.IsValidIndex(TargetSlotIndex) && IsWeaponQuickSlot(TargetSlotIndex))
					{
						QuickSlotItems[TargetSlotIndex].SetFromSlot(EquippedWeapon);
						bChanged = true;
					}
				}
			}
		}
	}

	for (int32 Index = 0; Index < QuickSlotItems.Num(); ++Index)
	{
		FPDQuickSlotReference& Slot = QuickSlotItems[Index];
		if (Slot.IsEmpty())
		{
			continue;
		}

		const FPDInventorySlot ResolvedSlot = ResolveQuickSlotData(Slot);
		if (ResolvedSlot.IsEmpty() || !CanAssignItemToSlot(ResolvedSlot.ItemData, Index))
		{
			Slot.Clear();
			bChanged = true;
			continue;
		}

		if (IsWeaponQuickSlot(Index) && Slot.ItemInstanceID.IsValid())
		{
			bool bSpecificItemAvailable = FindInventorySlotByInstanceID(Slot.ItemInstanceID) != INDEX_NONE;
			if (!bSpecificItemAvailable)
			{
				if (const UPDEquipmentComponent* EquipmentComponent = FindOwnerEquipment())
				{
					const FPDInventorySlot EquippedWeapon = EquipmentComponent->GetEquippedSlot(EPDEquipmentSlotType::Weapon);
					bSpecificItemAvailable = !EquippedWeapon.IsEmpty() && EquippedWeapon.ItemInstanceID == Slot.ItemInstanceID;
				}
			}

			if (!bSpecificItemAvailable)
			{
				Slot.Clear();
				bChanged = true;
				continue;
			}
		}

		if (GetAvailableItemQuantity(ResolvedSlot.ItemData) <= 0)
		{
			Slot.Clear();
			bChanged = true;
			continue;
		}
	}


	return bChanged;
}

void UPDQuickSlotComponent::HandleInventoryChanged()
{

	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		OnQuickSlotsChanged.Broadcast();
		return;
	}

	if (SyncQuickSlotsWithInventory())
	{
		OnQuickSlotsChanged.Broadcast();
	}
}

void UPDQuickSlotComponent::HandleEquipmentChanged()
{

	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		if (QuickSlotItems.Num() != GetMaxSlotCount())
		{
			QuickSlotItems.SetNum(GetMaxSlotCount());
		}
		const bool bChanged = SyncQuickSlotsWithInventory();
		OnQuickSlotsChanged.Broadcast();
		return;
	}

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
		return ItemData.WeaponType != EWeaponType::None ||
			(ItemData.ItemType == EPDItemType::Equipment && ItemData.EquipmentSlotType == EPDEquipmentSlotType::Weapon);
	}

	return IsConsumableQuickSlot(SlotIndex) && ItemData.ItemType == EPDItemType::Consumable;
}

FPDInventorySlot UPDQuickSlotComponent::ResolveQuickSlotData(const FPDQuickSlotReference& SlotRef) const
{
	FPDInventorySlot ResolvedSlot;
	if (SlotRef.IsEmpty())
	{
		return ResolvedSlot;
	}

	if (const UPDInventoryComponent* InventoryComponent = FindOwnerInventory())
	{
		if (const FPDInventorySlot* InventorySlot = InventoryComponent->FindSlotByInstanceID(SlotRef.ItemInstanceID))
		{
			ResolvedSlot = *InventorySlot;
			if (ResolvedSlot.ItemData.ItemType == EPDItemType::Consumable)
			{
				ResolvedSlot.Quantity = GetInventoryItemQuantity(SlotRef.ItemID);
			}
			return ResolvedSlot;
		}
	}

	if (const UPDEquipmentComponent* EquipmentComponent = FindOwnerEquipment())
	{
		const FPDInventorySlot EquippedWeapon = EquipmentComponent->GetEquippedSlot(EPDEquipmentSlotType::Weapon);
		const bool bSameInstance = SlotRef.ItemInstanceID.IsValid() &&
			EquippedWeapon.ItemInstanceID == SlotRef.ItemInstanceID;
		const bool bSameLegacyItem = !SlotRef.ItemInstanceID.IsValid() &&
			EquippedWeapon.ItemData.ItemID == SlotRef.ItemID;
		if (!EquippedWeapon.IsEmpty() && (bSameInstance || bSameLegacyItem))
		{
			return EquippedWeapon;
		}
	}

	if (const UPDInventoryComponent* InventoryComponent = FindOwnerInventory())
	{
		const int32 FallbackIndex = FindInventorySlotByItemID(SlotRef.ItemID);
		if (InventoryComponent->Items.IsValidIndex(FallbackIndex))
		{
			ResolvedSlot = InventoryComponent->Items[FallbackIndex];
			if (ResolvedSlot.ItemData.ItemType == EPDItemType::Consumable)
			{
				ResolvedSlot.Quantity = GetInventoryItemQuantity(SlotRef.ItemID);
			}
		}
	}

	return ResolvedSlot;
}

FPDInventorySlot UPDQuickSlotComponent::ResolvePendingConsumableSlot() const
{
	FPDInventorySlot ResolvedSlot;
	if (PendingConsumableItemID.IsNone())
	{
		return ResolvedSlot;
	}

	if (const UPDInventoryComponent* InventoryComponent = FindOwnerInventory())
	{
		if (const FPDInventorySlot* InventorySlot = InventoryComponent->FindSlotByInstanceID(PendingConsumableInstanceID))
		{
			return *InventorySlot;
		}

		const int32 FallbackIndex = FindInventorySlotByItemID(PendingConsumableItemID);
		if (InventoryComponent->Items.IsValidIndex(FallbackIndex))
		{
			return InventoryComponent->Items[FallbackIndex];
		}
	}

	return ResolvedSlot;
}

bool UPDQuickSlotComponent::GetQuickSlotDisplayData(int32 SlotIndex, FPDInventorySlot& OutSlotData) const
{
	OutSlotData = FPDInventorySlot();
	if (!QuickSlotItems.IsValidIndex(SlotIndex))
	{
		return false;
	}

	OutSlotData = ResolveQuickSlotData(QuickSlotItems[SlotIndex]);
	if (!QuickSlotItems[SlotIndex].IsEmpty() || !OutSlotData.IsEmpty())
	{
	}
	return !OutSlotData.IsEmpty();
}

const FPDInventorySlot* UPDQuickSlotComponent::GetResolvedQuickSlot(int32 SlotIndex) const
{
	if (!QuickSlotItems.IsValidIndex(SlotIndex))
	{
		return nullptr;
	}

	ResolvedQuickSlotScratch = ResolveQuickSlotData(QuickSlotItems[SlotIndex]);
	return ResolvedQuickSlotScratch.IsEmpty() ? nullptr : &ResolvedQuickSlotScratch;
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
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		return;
	}

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

	for (FPDQuickSlotReference& Slot : QuickSlotItems)
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
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		ServerResetQuickSlots();
		return;
	}

	QuickSlotItems.SetNum(GetMaxSlotCount());

	for (FPDQuickSlotReference& Slot : QuickSlotItems)
	{
		Slot.Clear();
	}

	OnQuickSlotsChanged.Broadcast();
}

bool UPDQuickSlotComponent::AddSlotReference(const FPDInventorySlot& SourceSlot)
{

	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		return false;
	}

	if (SourceSlot.IsEmpty())
	{
		return false;
	}

	if (QuickSlotItems.Num() != GetMaxSlotCount())
	{
		InitializeQuickSlots();
	}

	int32 TargetSlotIndex = SourceSlot.ItemInstanceID.IsValid()
		? FindWeaponQuickSlotByInstanceID(SourceSlot.ItemInstanceID)
		: INDEX_NONE;
	if (TargetSlotIndex == INDEX_NONE)
	{
		TargetSlotIndex = FindWeaponQuickSlotByItemID(SourceSlot.ItemData.ItemID);
	}
	if (TargetSlotIndex == INDEX_NONE)
	{
		TargetSlotIndex = FindEmptySlotForItem(SourceSlot.ItemData);
	}
	if (!QuickSlotItems.IsValidIndex(TargetSlotIndex) || !CanAssignItemToSlot(SourceSlot.ItemData, TargetSlotIndex))
	{
		return false;
	}

	for (int32 Index = 0; Index < QuickSlotItems.Num(); ++Index)
	{
		if (Index == TargetSlotIndex || QuickSlotItems[Index].IsEmpty())
		{
			continue;
		}

		const bool bSameInstance = SourceSlot.ItemInstanceID.IsValid() &&
			QuickSlotItems[Index].ItemInstanceID == SourceSlot.ItemInstanceID;
		const bool bSameLegacyItem = !SourceSlot.ItemInstanceID.IsValid() &&
			QuickSlotItems[Index].ItemID == SourceSlot.ItemData.ItemID;
		if (bSameInstance || bSameLegacyItem)
		{
			QuickSlotItems[Index].Clear();
		}
	}

	QuickSlotItems[TargetSlotIndex].SetFromSlot(SourceSlot);
	const bool bSyncedChanged = SyncQuickSlotsWithInventory();
	ForceQuickSlotReplication();
	OnQuickSlotsChanged.Broadcast();
	return true;
}

bool UPDQuickSlotComponent::MoveSlotQuantityToSlot(int32 SourceSlotIndex, int32 TargetSlotIndex, int32 Quantity)
{
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		ServerMoveSlotQuantityToSlot(SourceSlotIndex, TargetSlotIndex, Quantity);
		return true;
	}

	if (QuickSlotItems.Num() != GetMaxSlotCount())
	{
		InitializeQuickSlots();
	}

	if (!QuickSlotItems.IsValidIndex(SourceSlotIndex) || !QuickSlotItems.IsValidIndex(TargetSlotIndex) || SourceSlotIndex == TargetSlotIndex)
	{
		return false;
	}

	const FPDQuickSlotReference SourceSlot = QuickSlotItems[SourceSlotIndex];
	const FPDQuickSlotReference TargetSlot = QuickSlotItems[TargetSlotIndex];
	const FPDInventorySlot SourceSlotData = ResolveQuickSlotData(SourceSlot);
	const FPDInventorySlot TargetSlotData = ResolveQuickSlotData(TargetSlot);
	if (SourceSlotData.IsEmpty())
	{
		return false;
	}

	if (!CanAssignItemToSlot(SourceSlotData.ItemData, TargetSlotIndex))
	{
		return false;
	}

	if (!TargetSlot.IsEmpty() && (TargetSlotData.IsEmpty() || !CanAssignItemToSlot(TargetSlotData.ItemData, SourceSlotIndex)))
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

	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		ServerStoreInventorySlotQuantityToSlot(SourceInventory, SourceSlotIndex, TargetQuickSlotIndex, Quantity);
		return true;
	}

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
		const bool bSourceEquipmentLike = SourceSlot.ItemData.ItemType == EPDItemType::Equipment || SourceSlot.ItemData.WeaponType != EWeaponType::None;
		const bool bSameEquipmentInstance = bSourceEquipmentLike &&
			SourceSlot.ItemInstanceID.IsValid() &&
			QuickSlotItems[Index].ItemInstanceID == SourceSlot.ItemInstanceID;
		const bool bSameConsumableItem = !bSourceEquipmentLike &&
			QuickSlotItems[Index].ItemID == SourceSlot.ItemData.ItemID;
		const bool bSameLegacyEquipmentItem = bSourceEquipmentLike &&
			!SourceSlot.ItemInstanceID.IsValid() &&
			QuickSlotItems[Index].ItemID == SourceSlot.ItemData.ItemID;
		if (Index != TargetQuickSlotIndex && !QuickSlotItems[Index].IsEmpty() && (bSameEquipmentInstance || bSameConsumableItem || bSameLegacyEquipmentItem))
		{
			QuickSlotItems[Index].Clear();
		}
	}

	QuickSlotItems[TargetQuickSlotIndex].SetFromSlot(SourceSlot);

	const bool bSyncedChanged = SyncQuickSlotsWithInventory();
	ForceQuickSlotReplication();
	OnQuickSlotsChanged.Broadcast();
	return true;
}

bool UPDQuickSlotComponent::RemoveItemFromSlot(int32 SlotIndex, int32 Quantity)
{
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		ServerRemoveItemFromSlot(SlotIndex, Quantity);
		return true;
	}

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
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		ServerUseInventoryConsumableSlot(InventorySlotIndex);
		return true;
	}

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

	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		ServerEquipInventoryWeaponSlot(InventorySlotIndex);
		return true;
	}

	const UPDInventoryComponent* InventoryComponent = FindOwnerInventory();
	if (!InventoryComponent || !InventoryComponent->Items.IsValidIndex(InventorySlotIndex))
	{
		return false;
	}

	const FPDInventorySlot Slot = InventoryComponent->Items[InventorySlotIndex];
	if (Slot.IsEmpty() ||
		(Slot.ItemData.WeaponType == EWeaponType::None &&
			(Slot.ItemData.ItemType != EPDItemType::Equipment || Slot.ItemData.EquipmentSlotType != EPDEquipmentSlotType::Weapon)))
	{
		return false;
	}

	int32 CooldownSlotIndex = FindWeaponQuickSlotByInstanceID(Slot.ItemInstanceID);
	if (CooldownSlotIndex == INDEX_NONE)
	{
		CooldownSlotIndex = FindWeaponQuickSlotByItemID(Slot.ItemData.ItemID);
	}
	return EquipWeaponFromInventorySlot(InventorySlotIndex, CooldownSlotIndex, false);
}

bool UPDQuickSlotComponent::UseQuickSlot(int32 SlotIndex)
{

	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		ServerUseQuickSlot(SlotIndex);
		return true;
	}

	if (QuickSlotItems.Num() != GetMaxSlotCount())
	{
		InitializeQuickSlots();
	}

	if (!QuickSlotItems.IsValidIndex(SlotIndex))
	{
		return false;
	}

	const FPDQuickSlotReference SlotRef = QuickSlotItems[SlotIndex];
	const FPDInventorySlot SlotData = ResolveQuickSlotData(SlotRef);
	if (SlotData.IsEmpty() || !CanAssignItemToSlot(SlotData.ItemData, SlotIndex))
	{
		return false;
	}

	if (IsWeaponQuickSlot(SlotIndex))
	{
		return UseWeaponQuickSlot(SlotIndex, SlotRef, SlotData);
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

		return BeginConsumableUse(SlotIndex, SlotRef);
	}

	return false;
}

bool UPDQuickSlotComponent::UseWeaponQuickSlot(int32 SlotIndex, const FPDQuickSlotReference& SlotRef, const FPDInventorySlot& SlotData)
{

	if (SlotRef.IsEmpty() || SlotData.IsEmpty() || !IsWeaponQuickSlot(SlotIndex))
	{
		return false;
	}

	if (APDPlayerCharacter* PlayerCharacter = FindOwnerPlayerCharacter())
	{
		const EWeaponSlot WeaponSlot = PlayerCharacter->GetSlotForWeaponType(SlotData.ItemData.WeaponType);
		if (APDWeaponBase* Weapon = PlayerCharacter->GetWeaponInSlot(WeaponSlot))
		{
			const bool bSameInstance = SlotRef.ItemInstanceID.IsValid() &&
				Weapon->GetItemInstanceID().IsValid() &&
				Weapon->GetItemInstanceID() == SlotRef.ItemInstanceID;
			const bool bSameLegacyItem = !SlotRef.ItemInstanceID.IsValid() && Weapon->GetItemID() == SlotRef.ItemID;
			if (bSameInstance || bSameLegacyItem)
			{
				if (PlayerCharacter->GetCurrentSlot() == WeaponSlot)
				{
					SetSelectedIndex(SlotIndex);
					return true;
				}

				PlayerCharacter->SwitchToSlot(WeaponSlot);
				SetSelectedIndex(SlotIndex);
				StartWeaponQuickSlotCooldown(SlotIndex);
				return true;
			}
		}
	}

	int32 InventorySlotIndex = FindInventorySlotByInstanceID(SlotRef.ItemInstanceID);
	if (InventorySlotIndex == INDEX_NONE)
	{
		InventorySlotIndex = FindInventorySlotByItemID(SlotRef.ItemID);
	}
	if (InventorySlotIndex == INDEX_NONE)
	{
		const bool bSyncedChanged = SyncQuickSlotsWithInventory();
		OnQuickSlotsChanged.Broadcast();
		return false;
	}

	return EquipWeaponFromInventorySlot(InventorySlotIndex, SlotIndex, true);
}

bool UPDQuickSlotComponent::EquipWeaponFromInventorySlot(int32 InventorySlotIndex, int32 CooldownSlotIndex, bool bRespectWeaponSwitchCooldown)
{

	if (bIsUsingConsumable || (bRespectWeaponSwitchCooldown && IsWeaponQuickSlotOnCooldown()))
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
	if (InventorySlot.IsEmpty() ||
		(InventorySlot.ItemData.WeaponType == EWeaponType::None &&
			(InventorySlot.ItemData.ItemType != EPDItemType::Equipment || InventorySlot.ItemData.EquipmentSlotType != EPDEquipmentSlotType::Weapon)))
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

	const bool bSyncedChanged = SyncQuickSlotsWithInventory();
	OnQuickSlotsChanged.Broadcast();
	if (QuickSlotItems.IsValidIndex(CooldownSlotIndex))
	{
		StartWeaponQuickSlotCooldown(CooldownSlotIndex);
	}
	return true;
}

bool UPDQuickSlotComponent::BeginConsumableUse(int32 SlotIndex, const FPDQuickSlotReference& SlotRef)
{
	const FPDInventorySlot Slot = ResolveQuickSlotData(SlotRef);
	return BeginConsumableUse(SlotIndex, Slot);
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
	if (QuickSlotItems.IsValidIndex(SlotIndex))
	{
		SetSelectedIndex(SlotIndex);
	}

	if (Duration <= 0.f)
	{
		if (!SendConsumableUseGameplayEvent(SlotIndex, Slot))
		{
			SyncQuickSlotsWithInventory();
			OnQuickSlotsChanged.Broadcast();
			return false;
		}

		return true;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	bIsUsingConsumable = true;
	bLastConsumableUseCompleted = false;
	UsingConsumableSlotIndex = SlotIndex;
	PendingConsumableItemID = Slot.ItemData.ItemID;
	PendingConsumableInstanceID = Slot.ItemInstanceID;
	ConsumableUseStartTime = World->GetTimeSeconds();
	ConsumableUseEndTime = ConsumableUseStartTime + Duration;

	StartConsumableUseSound(Slot.ItemData);
	OnConsumableUseStarted.Broadcast(SlotIndex, Slot.ItemData, Duration);

	if (!SendConsumableUseGameplayEvent(SlotIndex, Slot))
	{
		NotifyConsumableAbilityCanceled(SlotIndex, Slot.ItemData);
		SyncQuickSlotsWithInventory();
		OnQuickSlotsChanged.Broadcast();
		return false;
	}

	return true;
}

bool UPDQuickSlotComponent::SendConsumableUseGameplayEvent(int32 SlotIndex, const FPDInventorySlot& Slot)
{
	AActor* EventTarget = GetOwner();
	if (APDPlayerCharacter* PlayerCharacter = FindOwnerPlayerCharacter())
	{
		EventTarget = PlayerCharacter;
	}

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(EventTarget);
	if (!ASC)
	{
		return false;
	}

	UPDConsumableUseRequest* Request = NewObject<UPDConsumableUseRequest>(this);
	Request->QuickSlotComponent = this;
	Request->ItemSlot = Slot;
	Request->QuickSlotIndex = SlotIndex;

	FGameplayEventData EventData;
	EventData.EventTag = PDGameplayTags::Event_Item_UseConsumable;
	EventData.Instigator = EventTarget;
	EventData.Target = EventTarget;
	EventData.OptionalObject = Request;

	return ASC->HandleGameplayEvent(PDGameplayTags::Event_Item_UseConsumable, &EventData) > 0;
}

bool UPDQuickSlotComponent::SendConsumableCancelGameplayEvent()
{
	AActor* EventTarget = GetOwner();
	if (APDPlayerCharacter* PlayerCharacter = FindOwnerPlayerCharacter())
	{
		EventTarget = PlayerCharacter;
	}

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(EventTarget);
	if (!ASC)
	{
		return false;
	}

	FGameplayEventData EventData;
	EventData.EventTag = PDGameplayTags::Event_Item_CancelConsumableUse;
	EventData.Instigator = EventTarget;
	EventData.Target = EventTarget;

	return ASC->HandleGameplayEvent(PDGameplayTags::Event_Item_CancelConsumableUse, &EventData) > 0;
}

void UPDQuickSlotComponent::NotifyConsumableAbilityCompleted(int32 SlotIndex, const FPDItemData& ItemData)
{
	bIsUsingConsumable = false;
	bLastConsumableUseCompleted = true;
	bConsumableUsePresentationActive = false;
	PresentedConsumableSlotIndex = INDEX_NONE;
	PresentedConsumableItemData = FPDItemData();
	UsingConsumableSlotIndex = INDEX_NONE;
	PendingConsumableItemID = NAME_None;
	PendingConsumableInstanceID = FGuid();
	ConsumableUseStartTime = 0.f;
	ConsumableUseEndTime = 0.f;
	StopConsumableUseSound();

	if (QuickSlotItems.IsValidIndex(SlotIndex))
	{
		SetSelectedIndex(SlotIndex);
	}

	SyncQuickSlotsWithInventory();
	OnQuickSlotsChanged.Broadcast();
	OnConsumableUseCompleted.Broadcast(SlotIndex, ItemData);
}

void UPDQuickSlotComponent::NotifyConsumableAbilityCanceled(int32 SlotIndex, const FPDItemData& ItemData)
{
	bIsUsingConsumable = false;
	bLastConsumableUseCompleted = false;
	bConsumableUsePresentationActive = false;
	PresentedConsumableSlotIndex = INDEX_NONE;
	PresentedConsumableItemData = FPDItemData();
	UsingConsumableSlotIndex = INDEX_NONE;
	PendingConsumableItemID = NAME_None;
	PendingConsumableInstanceID = FGuid();
	ConsumableUseStartTime = 0.f;
	ConsumableUseEndTime = 0.f;
	StopConsumableUseSound();
	OnConsumableUseCanceled.Broadcast(SlotIndex, ItemData);
}

bool UPDQuickSlotComponent::CancelConsumableUse()
{
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		StopConsumableUseSound();
		ServerCancelConsumableUse();
		return true;
	}

	if (!bIsUsingConsumable)
	{
		return false;
	}

	const int32 CanceledSlotIndex = UsingConsumableSlotIndex;
	const FPDInventorySlot CanceledSlot = ResolvePendingConsumableSlot();
	const FPDItemData CanceledItemData = CanceledSlot.ItemData;

	if (SendConsumableCancelGameplayEvent())
	{
		return true;
	}

	NotifyConsumableAbilityCanceled(CanceledSlotIndex, CanceledItemData);
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
	if (!World)
	{
		return 0.f;
	}

	const AActor* OwnerActor = GetOwner();
	const float CooldownEndTime = OwnerActor && OwnerActor->HasAuthority() ? WeaponCooldownEndTime : LocalWeaponCooldownEndTime;
	return FMath::Max(0.f, CooldownEndTime - World->GetTimeSeconds());
}

void UPDQuickSlotComponent::StartConsumableUseSound(const FPDItemData& ItemData)
{
	StopConsumableUseSound();
	ConsumableUseAudioComponent = UPDItemSoundLibrary::SpawnConsumableUseSound(this, ItemData);
}

void UPDQuickSlotComponent::StopConsumableUseSound()
{
	if (!IsValid(ConsumableUseAudioComponent))
	{
		ConsumableUseAudioComponent = nullptr;
		return;
	}

	ConsumableUseAudioComponent->Stop();
	ConsumableUseAudioComponent = nullptr;
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
	LocalWeaponCooldownEndTime = WeaponCooldownEndTime;
	bWeaponCooldownPresentationActive = true;
	PresentedWeaponCooldownSlotIndex = SlotIndex;
	World->GetTimerManager().SetTimer(WeaponQuickSlotCooldownTimerHandle, this, &UPDQuickSlotComponent::FinishWeaponQuickSlotCooldown, WeaponSwitchCooldown, false);
	OnWeaponQuickSlotCooldownStarted.Broadcast(SlotIndex, WeaponSwitchCooldown, WeaponCooldownEndTime);
	OnQuickSlotsChanged.Broadcast();
	ForceQuickSlotReplication();
}

void UPDQuickSlotComponent::FinishWeaponQuickSlotCooldown()
{
	const int32 FinishedSlotIndex = WeaponCooldownSlotIndex;
	bWeaponQuickSlotCooldownActive = false;
	WeaponCooldownSlotIndex = INDEX_NONE;
	WeaponCooldownEndTime = 0.f;
	LocalWeaponCooldownEndTime = 0.f;
	bWeaponCooldownPresentationActive = false;
	PresentedWeaponCooldownSlotIndex = INDEX_NONE;
	OnWeaponQuickSlotCooldownFinished.Broadcast(FinishedSlotIndex);
	OnQuickSlotsChanged.Broadcast();
	ForceQuickSlotReplication();
}

bool UPDQuickSlotComponent::HasItem(FName ItemID, int32 Quantity) const
{
	if (ItemID.IsNone() || Quantity <= 0)
	{
		return false;
	}

	int32 FoundQuantity = 0;

	for (const FPDQuickSlotReference& Slot : QuickSlotItems)
	{
		if (Slot.IsEmpty())
		{
			continue;
		}

		if (Slot.ItemID == ItemID)
		{
			const FPDInventorySlot ResolvedSlot = ResolveQuickSlotData(Slot);
			FoundQuantity += FMath::Max(0, ResolvedSlot.Quantity);

			if (FoundQuantity >= Quantity)
			{
				return true;
			}
		}
	}

	return false;
}
