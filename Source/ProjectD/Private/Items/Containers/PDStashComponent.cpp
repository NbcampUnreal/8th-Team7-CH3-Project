#include "Items/Containers/PDStashComponent.h"

#include "Core/PDGameInstance.h"
#include "Core/PDPlayerController.h"
#include "Core/PDPlayerState.h"
#include "GameFramework/Actor.h"
#include "Items/Data/PDItemSlotTransfer.h"
#include "Net/UnrealNetwork.h"

namespace
{
	APDPlayerController* GetLocalPDPlayerController(const UObject* WorldContext)
	{
		const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
		return World ? Cast<APDPlayerController>(World->GetFirstPlayerController()) : nullptr;
	}

	APDPlayerState* GetPDPlayerStateFromInventory(const UPDInventoryComponent* InventoryComponent)
	{
		return InventoryComponent ? Cast<APDPlayerState>(InventoryComponent->GetOwner()) : nullptr;
	}

	void ForceReplicationForChangedContainers(const UPDStashComponent* StashComponent, const UPDInventoryComponent* InventoryComponent)
	{
		if (AActor* StashOwner = StashComponent ? StashComponent->GetOwner() : nullptr)
		{
			StashOwner->FlushNetDormancy();
			StashOwner->ForceNetUpdate();
		}

		if (AActor* InventoryOwner = InventoryComponent ? InventoryComponent->GetOwner() : nullptr)
		{
			InventoryOwner->FlushNetDormancy();
			InventoryOwner->ForceNetUpdate();
		}
	}
}

UPDStashComponent::UPDStashComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	GridColumns = 5;
	BaseGridRows = 4;
	GridRows = BaseGridRows;
	CurrentUpgradeLevel = 0;

	UpgradeData.SetNum(3);
	UpgradeData[0].Cost = 1000;
	UpgradeData[0].AddedRows = 1;
	UpgradeData[1].Cost = 2500;
	UpgradeData[1].AddedRows = 1;
	UpgradeData[2].Cost = 5000;
	UpgradeData[2].AddedRows = 1;
}

void UPDStashComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UPDStashComponent, StashItems);
	DOREPLIFETIME(UPDStashComponent, GridRows);
	DOREPLIFETIME(UPDStashComponent, CurrentUpgradeLevel);
}

void UPDStashComponent::OnRep_StashItems()
{
	OnStashChanged.Broadcast();
}

void UPDStashComponent::OnRep_StashConfig()
{
	OnStashChanged.Broadcast();
	OnStashUpgraded.Broadcast(CurrentUpgradeLevel, GridRows);
}

void UPDStashComponent::BeginPlay()
{
	Super::BeginPlay();

	if (PersistenceMode == EPDStashPersistenceMode::GameInstanceLegacy)
	{
		LoadFromGameInstance();
	}
	else if (PersistenceMode == EPDStashPersistenceMode::PlayerStatePersistent)
	{
		SetStashUpgradeLevel(CurrentUpgradeLevel);
		InitializeStash();
	}
	else if (GetOwner() && GetOwner()->HasAuthority())
	{
		SetStashUpgradeLevel(CurrentUpgradeLevel);
		InitializeStash();
	}
}

void UPDStashComponent::LoadFromGameInstance()
{
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		return;
	}

	if (PersistenceMode != EPDStashPersistenceMode::GameInstanceLegacy)
	{
		SetStashUpgradeLevel(CurrentUpgradeLevel);
		InitializeStash();
		return;
	}

	const UPDGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance<UPDGameInstance>() : nullptr;
	if (GI)
	{
		StashItems = GI->GetStashItems();
		SetStashUpgradeLevel(GI->GetStashUpgradeLevel());
	}
	else
	{
		SetStashUpgradeLevel(CurrentUpgradeLevel);
	}

	InitializeStash();
}

void UPDStashComponent::LoadFromPlayerState(APDPlayerState* PlayerState)
{
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		return;
	}

	if (PersistenceMode != EPDStashPersistenceMode::PlayerStatePersistent || !PlayerState)
	{
		return;
	}

	StashItems = PlayerState->GetPersistentStashItems();
	SetStashUpgradeLevel(PlayerState->GetPersistentStashUpgradeLevel());
	InitializeStash();
}

void UPDStashComponent::SaveToGameInstance()
{
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		return;
	}

	if (PersistenceMode != EPDStashPersistenceMode::GameInstanceLegacy)
	{
		return;
	}

	UPDGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance<UPDGameInstance>() : nullptr;
	if (!GI)
	{
		return;
	}

	GI->SetStashItems(StashItems);
	GI->SetStashUpgradeLevel(CurrentUpgradeLevel);
}

void UPDStashComponent::SaveToPlayerState(APDPlayerState* PlayerState, bool bSaveToDisk)
{
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		return;
	}

	if (PersistenceMode != EPDStashPersistenceMode::PlayerStatePersistent || !PlayerState)
	{
		return;
	}

	PlayerState->SetPersistentStashSnapshot(StashItems, CurrentUpgradeLevel);

	if (!bSaveToDisk)
	{
		return;
	}

	if (UPDGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance<UPDGameInstance>() : nullptr)
	{
		GI->SavePlayerDataToDisk(
			GI->GetSaveKeyForController(Cast<APlayerController>(PlayerState->GetOwner())),
			PlayerState->GetPersistentData());
	}
}

void UPDStashComponent::PersistIfNeeded()
{
	if (PersistenceMode == EPDStashPersistenceMode::GameInstanceLegacy)
	{
		SaveToGameInstance();
	}
}

void UPDStashComponent::PersistIfNeededForInventory(const UPDInventoryComponent* InventoryComponent)
{
	if (PersistenceMode == EPDStashPersistenceMode::PlayerStatePersistent)
	{
		SaveToPlayerState(GetPDPlayerStateFromInventory(InventoryComponent));
		return;
	}

	PersistIfNeeded();
}

int32 UPDStashComponent::GetNextUpgradeCost() const
{
	return UpgradeData.IsValidIndex(CurrentUpgradeLevel) ? FMath::Max(0, UpgradeData[CurrentUpgradeLevel].Cost) : 0;
}

int32 UPDStashComponent::GetNextUpgradeAddedRows() const
{
	return UpgradeData.IsValidIndex(CurrentUpgradeLevel) ? FMath::Max(0, UpgradeData[CurrentUpgradeLevel].AddedRows) : 0;
}

EPDStashUpgradeResult UPDStashComponent::CanUpgradeStash(const UPDInventoryComponent* SourceInventory) const
{
	if (!SourceInventory)
	{
		return EPDStashUpgradeResult::InvalidInventory;
	}

	if (IsMaxUpgradeLevel())
	{
		return EPDStashUpgradeResult::AlreadyMaxLevel;
	}

	const int32 UpgradeCost = GetNextUpgradeCost();
	const int32 AddedRows = GetNextUpgradeAddedRows();
	if (UpgradeCost <= 0 || AddedRows <= 0)
	{
		return EPDStashUpgradeResult::InvalidConfig;
	}

	if (SourceInventory->GetGold() < UpgradeCost)
	{
		return EPDStashUpgradeResult::NotEnoughGold;
	}

	return EPDStashUpgradeResult::Success;
}

EPDStashUpgradeResult UPDStashComponent::UpgradeStash(UPDInventoryComponent* SourceInventory)
{
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		if (APDPlayerController* PC = GetLocalPDPlayerController(this))
		{
			PC->ServerUpgradeStash(this);
			return EPDStashUpgradeResult::Success;
		}
		return EPDStashUpgradeResult::InvalidInventory;
	}

	const EPDStashUpgradeResult Result = CanUpgradeStash(SourceInventory);
	if (Result != EPDStashUpgradeResult::Success)
	{
		OnStashUpgradeFailed.Broadcast(Result);
		return Result;
	}

	const int32 UpgradeCost = GetNextUpgradeCost();
	const int32 AddedRows = GetNextUpgradeAddedRows();

	if (!SourceInventory->SpendGold(UpgradeCost))
	{
		OnStashUpgradeFailed.Broadcast(EPDStashUpgradeResult::NotEnoughGold);
		return EPDStashUpgradeResult::NotEnoughGold;
	}

	++CurrentUpgradeLevel;
	GridRows += AddedRows;

	InitializeStash();
	PersistIfNeededForInventory(SourceInventory);
	OnStashUpgraded.Broadcast(CurrentUpgradeLevel, GridRows);
	return EPDStashUpgradeResult::Success;
}

void UPDStashComponent::SetStashUpgradeLevel(int32 NewUpgradeLevel)
{
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		return;
	}

	CurrentUpgradeLevel = FMath::Clamp(NewUpgradeLevel, 0, GetMaxUpgradeLevel());

	GridRows = FMath::Max(1, BaseGridRows);
	for (int32 Index = 0; Index < CurrentUpgradeLevel && UpgradeData.IsValidIndex(Index); ++Index)
	{
		GridRows += FMath::Max(0, UpgradeData[Index].AddedRows);
	}
}

int32 UPDStashComponent::FindEmptySlot() const
{
	return FPDItemContainerOps::FindEmptySlot(StashItems);
}

void UPDStashComponent::InitializeStash()
{
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		return;
	}

	const int32 MaxSlotCount = GetMaxSlotCount();

	if (MaxSlotCount <= 0)
	{
		StashItems.Empty();
		OnStashChanged.Broadcast();
		return;
	}

	const int32 OldCount = StashItems.Num();

	StashItems.SetNum(MaxSlotCount);

	for (int32 Index = OldCount; Index < StashItems.Num(); ++Index)
	{
		StashItems[Index].Clear();
	}

	for (FPDInventorySlot& Slot : StashItems)
	{
		if (Slot.IsEmpty())
		{
			Slot.Clear();
		}
	}

	FPDItemContainerOps::EnsureInstanceIDs(StashItems);
	OnStashChanged.Broadcast();
}

void UPDStashComponent::ResetStash()
{
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		return;
	}

	StashItems.SetNum(GetMaxSlotCount());

	for (FPDInventorySlot& Slot : StashItems)
	{
		Slot.Clear();
	}

	PersistIfNeeded();
	OnStashChanged.Broadcast();
}

int32 UPDStashComponent::AddItemPartial(const FPDItemData& ItemData, int32 Quantity)
{
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		return 0;
	}

	if (ItemData.ItemID.IsNone() || Quantity <= 0)
	{
		return 0;
	}

	if (StashItems.Num() != GetMaxSlotCount())
	{
		InitializeStash();
	}

	const int32 AddedQuantity = FPDItemContainerOps::AddItem(StashItems, ItemData, Quantity);
	if (AddedQuantity > 0)
	{
		PersistIfNeeded();
		OnStashChanged.Broadcast();
		ForceReplicationForChangedContainers(this, nullptr);
	}

	return AddedQuantity;
}


bool UPDStashComponent::AddItemByID(FName ItemID, int32 Quantity)
{
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		return false;
	}

	if (!ItemDataTable || ItemID.IsNone()) return false;

	TArray<FPDItemData*> Rows;
	ItemDataTable->GetAllRows<FPDItemData>(TEXT("AddItemByID"), Rows);

	for (const FPDItemData* Row : Rows)
	{
		if (Row && Row->ItemID == ItemID)
			return AddItemPartial(*Row, Quantity) > 0;
	}

	return false;
}

int32 UPDStashComponent::AddItemToSlotPartial(const FPDItemData& ItemData, int32 Quantity, int32 TargetSlotIndex)
{
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		return 0;
	}

	if (StashItems.Num() != GetMaxSlotCount())
	{
		InitializeStash();
	}

	if (!StashItems.IsValidIndex(TargetSlotIndex))
	{
		return 0;
	}

	const int32 AddedQuantity = FPDItemSlotTransfer::AddItemToSlot(StashItems[TargetSlotIndex], ItemData, Quantity);
	if (AddedQuantity > 0)
	{
		PersistIfNeeded();
		OnStashChanged.Broadcast();
		ForceReplicationForChangedContainers(this, nullptr);
	}

	return AddedQuantity;
}

bool UPDStashComponent::MoveSlotQuantityToSlot(int32 SourceSlotIndex, int32 TargetSlotIndex, int32 Quantity)
{
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		if (APDPlayerController* PC = GetLocalPDPlayerController(this))
		{
			PC->ServerMoveStashSlotQuantity(this, SourceSlotIndex, TargetSlotIndex, Quantity);
			return true;
		}
		return false;
	}

	if (StashItems.Num() != GetMaxSlotCount())
	{
		InitializeStash();
	}

	if (!StashItems.IsValidIndex(SourceSlotIndex) || !StashItems.IsValidIndex(TargetSlotIndex) || SourceSlotIndex == TargetSlotIndex || Quantity <= 0)
	{
		return false;
	}

	const bool bMoved = FPDItemSlotTransfer::MoveQuantity(StashItems[SourceSlotIndex], StashItems[TargetSlotIndex], Quantity);
	if (bMoved)
	{
		PersistIfNeeded();
		OnStashChanged.Broadcast();
		ForceReplicationForChangedContainers(this, nullptr);
	}

	return bMoved;
}

bool UPDStashComponent::RemoveItem(FName ItemID, int32 Quantity)
{
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		return false;
	}

	if (!FPDItemContainerOps::RemoveItem(StashItems, ItemID, Quantity))
	{
		return false;
	}

	PersistIfNeeded();
	OnStashChanged.Broadcast();
	ForceReplicationForChangedContainers(this, nullptr);
	return true;
}

bool UPDStashComponent::StoreInventorySlot(UPDInventoryComponent* SourceInventory, int32 SourceSlotIndex)
{
	if (!SourceInventory || !SourceInventory->Items.IsValidIndex(SourceSlotIndex))
	{
		return false;
	}

	const FPDInventorySlot& SourceSlot = SourceInventory->Items[SourceSlotIndex];
	return StoreInventorySlotQuantity(SourceInventory, SourceSlotIndex, SourceSlot.IsEmpty() ? 0 : SourceSlot.Quantity);
}

bool UPDStashComponent::StoreInventorySlotQuantity(UPDInventoryComponent* SourceInventory, int32 SourceSlotIndex, int32 Quantity)
{
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		if (APDPlayerController* PC = GetLocalPDPlayerController(this))
		{
			PC->ServerStoreInventorySlotQuantityToStash(this, SourceSlotIndex, INDEX_NONE, Quantity);
			return true;
		}
		return false;
	}

	if (!SourceInventory || !SourceInventory->Items.IsValidIndex(SourceSlotIndex) || Quantity <= 0)
	{
		return false;
	}

	FPDInventorySlot& SourceSlot = SourceInventory->Items[SourceSlotIndex];

	if (SourceSlot.IsEmpty())
	{
		return false;
	}

	const int32 MoveQuantity = FMath::Min(Quantity, SourceSlot.Quantity);
	FPDInventorySlot TransferSlot = SourceSlot;
	TransferSlot.Quantity = MoveQuantity;
	TransferSlot.bIsEmpty = false;
	const int32 AddedQuantity = FPDItemContainerOps::AddSlot(StashItems, TransferSlot);

	if (AddedQuantity <= 0)
	{
		return false;
	}

	SourceSlot.Quantity -= AddedQuantity;

	if (SourceSlot.Quantity <= 0)
	{
		SourceSlot.Clear();
	}

	SourceInventory->OnInventoryChanged.Broadcast();
	OnStashChanged.Broadcast();
	PersistIfNeededForInventory(SourceInventory);
	ForceReplicationForChangedContainers(this, SourceInventory);
	return true;
}


bool UPDStashComponent::StoreInventorySlotQuantityToSlot(UPDInventoryComponent* SourceInventory, int32 SourceSlotIndex, int32 TargetStashSlotIndex, int32 Quantity)
{
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		if (APDPlayerController* PC = GetLocalPDPlayerController(this))
		{
			PC->ServerStoreInventorySlotQuantityToStash(this, SourceSlotIndex, TargetStashSlotIndex, Quantity);
			return true;
		}
		return false;
	}

	if (!SourceInventory || SourceInventory->Items.Num() != SourceInventory->GetMaxSlotCount())
	{
		if (SourceInventory)
		{
			SourceInventory->InitializeInventory();
		}
	}

	if (StashItems.Num() != GetMaxSlotCount())
	{
		InitializeStash();
	}

	if (!SourceInventory || !SourceInventory->Items.IsValidIndex(SourceSlotIndex) || !StashItems.IsValidIndex(TargetStashSlotIndex) || Quantity <= 0)
	{
		return false;
	}

	const bool bMoved = FPDItemSlotTransfer::MoveQuantity(SourceInventory->Items[SourceSlotIndex], StashItems[TargetStashSlotIndex], Quantity);
	if (bMoved)
	{
		SourceInventory->OnInventoryChanged.Broadcast();
		PersistIfNeededForInventory(SourceInventory);
		OnStashChanged.Broadcast();
		ForceReplicationForChangedContainers(this, SourceInventory);
	}

	return bMoved;
}

bool UPDStashComponent::TakeStashSlot(UPDInventoryComponent* TargetInventory, int32 StashSlotIndex)
{
	if (!StashItems.IsValidIndex(StashSlotIndex))
	{
		return false;
	}

	const FPDInventorySlot& SourceSlot = StashItems[StashSlotIndex];
	return TakeStashSlotQuantity(TargetInventory, StashSlotIndex, SourceSlot.IsEmpty() ? 0 : SourceSlot.Quantity);
}

bool UPDStashComponent::TakeStashSlotQuantity(UPDInventoryComponent* TargetInventory, int32 StashSlotIndex, int32 Quantity)
{
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		if (APDPlayerController* PC = GetLocalPDPlayerController(this))
		{
			PC->ServerTakeStashSlotQuantity(this, StashSlotIndex, Quantity);
			return true;
		}
		return false;
	}

	if (!TargetInventory || !StashItems.IsValidIndex(StashSlotIndex) || Quantity <= 0)
	{
		return false;
	}

	FPDInventorySlot& SourceSlot = StashItems[StashSlotIndex];

	if (SourceSlot.IsEmpty())
	{
		return false;
	}

	const int32 MoveQuantity = FMath::Min(Quantity, SourceSlot.Quantity);
	FPDInventorySlot TransferSlot = SourceSlot;
	TransferSlot.Quantity = MoveQuantity;

	const int32 AddedQuantity = TargetInventory->AddSlotPartial(TransferSlot);

	if (AddedQuantity <= 0)
	{
		return false;
	}

	SourceSlot.Quantity -= AddedQuantity;

	if (SourceSlot.Quantity <= 0)
	{
		SourceSlot.Clear();
	}

	PersistIfNeededForInventory(TargetInventory);
	OnStashChanged.Broadcast();
	ForceReplicationForChangedContainers(this, TargetInventory);
	return true;
}


bool UPDStashComponent::TakeStashSlotQuantityToInventorySlot(UPDInventoryComponent* TargetInventory, int32 StashSlotIndex, int32 TargetInventorySlotIndex, int32 Quantity)
{
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		if (APDPlayerController* PC = GetLocalPDPlayerController(this))
		{
			PC->ServerTakeStashSlotQuantityToInventorySlot(this, StashSlotIndex, TargetInventorySlotIndex, Quantity);
			return true;
		}
		return false;
	}

	if (!TargetInventory || TargetInventory->Items.Num() != TargetInventory->GetMaxSlotCount())
	{
		if (TargetInventory)
		{
			TargetInventory->InitializeInventory();
		}
	}

	if (StashItems.Num() != GetMaxSlotCount())
	{
		InitializeStash();
	}

	if (!TargetInventory || !StashItems.IsValidIndex(StashSlotIndex) || !TargetInventory->Items.IsValidIndex(TargetInventorySlotIndex) || Quantity <= 0)
	{
		return false;
	}

	const FPDInventorySlot& SourceSlot = StashItems[StashSlotIndex];
	if (!TargetInventory->CanAddSlotWeight(SourceSlot, Quantity))
	{
		TargetInventory->BroadcastWeightLimitExceeded();
		return false;
	}

	const bool bMoved = FPDItemSlotTransfer::MoveQuantity(StashItems[StashSlotIndex], TargetInventory->Items[TargetInventorySlotIndex], Quantity);
	if (bMoved)
	{
		PersistIfNeededForInventory(TargetInventory);
		OnStashChanged.Broadcast();
		TargetInventory->OnInventoryChanged.Broadcast();
		ForceReplicationForChangedContainers(this, TargetInventory);
	}

	return bMoved;
}

bool UPDStashComponent::HasItem(FName ItemID, int32 Quantity) const
{
	return FPDItemContainerOps::HasItem(StashItems, ItemID, Quantity);
}
