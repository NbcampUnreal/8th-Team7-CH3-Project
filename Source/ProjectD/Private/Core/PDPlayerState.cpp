#include "Core/PDPlayerState.h"

#include "Characters/PDPlayerCharacter.h"
#include "Items/Containers/PDInventoryComponent.h"
#include "Items/Equipment/PDEquipmentComponent.h"
#include "Items/Data/PDItemSlotTransfer.h"
#include "Items/Containers/PDQuickSlotComponent.h"
#include "Data/PDQuestComponent.h"
#include "Engine/DataTable.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

APDPlayerState::APDPlayerState()
{
	bReplicates = true;

	InventoryComponent = CreateDefaultSubobject<UPDInventoryComponent>(TEXT("InventoryComponent"));
	EquipmentComponent = CreateDefaultSubobject<UPDEquipmentComponent>(TEXT("EquipmentComponent"));
	QuickSlotComponent = CreateDefaultSubobject<UPDQuickSlotComponent>(TEXT("QuickSlotComponent"));
	QuestComponent = CreateDefaultSubobject<UPDQuestComponent>(TEXT("QuestComponent"));

	static ConstructorHelpers::FObjectFinder<UDataTable> ItemDataTableAsset(TEXT("/Game/Main/Data/DT_ItemData.DT_ItemData"));
	if (ItemDataTableAsset.Succeeded())
	{
		ItemDataTable = ItemDataTableAsset.Object;
	}

	ApplyComponentDataDefaults();
}

void APDPlayerState::BeginPlay()
{
	Super::BeginPlay();

	ApplyComponentDataDefaults();
}

void APDPlayerState::ApplyComponentDataDefaults()
{
	if (InventoryComponent && !InventoryComponent->ItemDataTable)
	{
		InventoryComponent->ItemDataTable = ItemDataTable;
	}
}


UPDQuickSlotComponent* APDPlayerState::GetQuickSlotComponent() const
{
	if (QuickSlotComponent && QuickSlotComponent->GetOwner() == this && !QuickSlotComponent->IsTemplate())
	{
		return QuickSlotComponent;
	}

	UPDQuickSlotComponent* RuntimeQuickSlotComponent = const_cast<APDPlayerState*>(this)->FindComponentByClass<UPDQuickSlotComponent>();
	if (RuntimeQuickSlotComponent && RuntimeQuickSlotComponent->GetOwner() == this && !RuntimeQuickSlotComponent->IsTemplate())
	{
		const_cast<APDPlayerState*>(this)->QuickSlotComponent = RuntimeQuickSlotComponent;
		return RuntimeQuickSlotComponent;
	}

	return QuickSlotComponent;
}

void APDPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(APDPlayerState, PersistentData, COND_OwnerOnly);
	DOREPLIFETIME(APDPlayerState, PersistentSaveId);
	DOREPLIFETIME(APDPlayerState, bIsExtracted);
	DOREPLIFETIME(APDPlayerState, bIsRaidDead);
	DOREPLIFETIME(APDPlayerState, RaidStats);
	DOREPLIFETIME(APDPlayerState, bIsTravelReady);
}

void APDPlayerState::SetExtracted(bool bInExtracted)
{
	if (!HasAuthority()) return;
	if (bIsExtracted == bInExtracted) return;
	bIsExtracted = bInExtracted;
	OnRaidParticipationChanged.Broadcast(bIsExtracted, bIsRaidDead);
	ForceNetUpdate();
}

void APDPlayerState::SetRaidDead(bool bInDead)
{
	if (!HasAuthority()) return;
	if (bIsRaidDead == bInDead) return;
	bIsRaidDead = bInDead;
	OnRaidParticipationChanged.Broadcast(bIsExtracted, bIsRaidDead);
	ForceNetUpdate();
}

void APDPlayerState::ResetRaidParticipationState()
{
	if (!HasAuthority()) return;

	// ?�탯/?�냅?�도 리셋 ??StartRaid 마다 ???�이??
	RaidStats = FPDRaidStats{};
	RaidInitialGold = 0;
	RaidInitialItemQuantity = 0;

	// 결산 ACK 도 리셋.
	if (bIsTravelReady)
	{
		bIsTravelReady = false;
		OnTravelReadyChanged.Broadcast(false);
	}

	if (!bIsExtracted && !bIsRaidDead)
	{
		ForceNetUpdate();
		return;
	}
	bIsExtracted = false;
	bIsRaidDead = false;
	OnRaidParticipationChanged.Broadcast(bIsExtracted, bIsRaidDead);
	ForceNetUpdate();
}

void APDPlayerState::SetRaidStats(const FPDRaidStats& InStats)
{
	if (!HasAuthority()) return;
	RaidStats = InStats;
	ForceNetUpdate();
}

void APDPlayerState::AddKill()
{
	if (!HasAuthority()) return;
	++RaidStats.Kills;
	ForceNetUpdate();
}

void APDPlayerState::SetSurvivalSeconds(float Seconds)
{
	if (!HasAuthority()) return;
	RaidStats.SurvivalSeconds = FMath::Max(0.f, Seconds);
	ForceNetUpdate();
}

void APDPlayerState::SetGoldDelta(int32 Delta)
{
	if (!HasAuthority()) return;
	RaidStats.GoldDelta = Delta;
	ForceNetUpdate();
}

void APDPlayerState::SetItemDelta(int32 Delta)
{
	if (!HasAuthority()) return;
	RaidStats.ItemDelta = Delta;
	ForceNetUpdate();
}

void APDPlayerState::CaptureInitialRaidSnapshot(int32 InGold, int32 InItemQuantity)
{
	if (!HasAuthority()) return;
	RaidInitialGold = FMath::Max(0, InGold);
	RaidInitialItemQuantity = FMath::Max(0, InItemQuantity);
}

void APDPlayerState::OnRep_RaidParticipation()
{
	OnRaidParticipationChanged.Broadcast(bIsExtracted, bIsRaidDead);
}

void APDPlayerState::SetTravelReady(bool bInReady)
{
	if (!HasAuthority()) return;
	if (bIsTravelReady == bInReady) return;
	bIsTravelReady = bInReady;
	OnTravelReadyChanged.Broadcast(bIsTravelReady);
	ForceNetUpdate();
}

void APDPlayerState::OnRep_TravelReady()
{
	OnTravelReadyChanged.Broadcast(bIsTravelReady);
}

APDPlayerCharacter* APDPlayerState::GetPDPlayerCharacter() const
{
	return GetPawn<APDPlayerCharacter>();
}

void APDPlayerState::OnRep_PersistentData()
{
}

void APDPlayerState::InitializePersistentData(const FPDPlayerData& InData)
{
	PersistentData = InData;
	FPDItemContainerOps::EnsureInstanceIDs(PersistentData.StashItems);
	FPDItemContainerOps::EnsureInstanceIDs(PersistentData.RaidLoadout);
	ForceNetUpdate();
}

void APDPlayerState::EnsurePersistentSaveId()
{
	if (!HasAuthority()) return;
	if (!PersistentSaveId.IsEmpty())
	{
		return;
	}

	PersistentSaveId = FGuid::NewGuid().ToString(EGuidFormats::Digits);
	ForceNetUpdate();
}

void APDPlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);

	// seamless travel: this(이전 PS) → 새 PS 로 안정 키 + 영구 데이터 보존.
	if (APDPlayerState* NewPS = Cast<APDPlayerState>(PlayerState))
	{
		NewPS->PersistentSaveId = PersistentSaveId;
		NewPS->PersistentData = PersistentData;
	}
}

int32 APDPlayerState::GetTraderReputationExp() const
{
	return FMath::Max(0, PersistentData.TraderReputationExp);
}

int32 APDPlayerState::GetTraderReputationLevel() const
{
	return FMath::Max(1, PersistentData.TraderReputationLevel);
}

void APDPlayerState::SetTraderReputation(int32 InLevel, int32 InExp)
{
	PersistentData.TraderReputationLevel = FMath::Max(1, InLevel);
	PersistentData.TraderReputationExp = FMath::Max(0, InExp);
	ForceNetUpdate();
}

void APDPlayerState::AddTraderReputationExp(int32 Amount)
{
	if (Amount <= 0)
	{
		return;
	}

	PersistentData.TraderReputationExp = FMath::Max(0, PersistentData.TraderReputationExp + Amount);
	ForceNetUpdate();
}

void APDPlayerState::ConfirmRaidLoadout(const TArray<FPDInventorySlot>& InLoadout, int32 InGold)
{
	PersistentData.RaidLoadout = InLoadout;
	FPDItemContainerOps::EnsureInstanceIDs(PersistentData.RaidLoadout);
	PersistentData.RaidGold = FMath::Max(0, InGold);
	PersistentData.Gold = FMath::Max(0, PersistentData.Gold - PersistentData.RaidGold);

	for (const FPDInventorySlot& LoadoutSlot : PersistentData.RaidLoadout)
	{
		if (LoadoutSlot.IsEmpty())
		{
			continue;
		}

		int32 QuantityToRemove = LoadoutSlot.Quantity;
		if (LoadoutSlot.ItemInstanceID.IsValid())
		{
			for (FPDInventorySlot& StashSlot : PersistentData.StashItems)
			{
				if (StashSlot.IsEmpty() || StashSlot.ItemInstanceID != LoadoutSlot.ItemInstanceID)
				{
					continue;
				}

				const int32 RemovedQuantity = FMath::Min(StashSlot.Quantity, QuantityToRemove);
				StashSlot.Quantity -= RemovedQuantity;
				QuantityToRemove -= RemovedQuantity;

				if (StashSlot.Quantity <= 0)
				{
					StashSlot.Clear();
				}

				break;
			}
		}

		for (FPDInventorySlot& StashSlot : PersistentData.StashItems)
		{
			if (QuantityToRemove <= 0)
			{
				break;
			}

			if (StashSlot.IsEmpty() || StashSlot.ItemData.ItemID != LoadoutSlot.ItemData.ItemID)
			{
				continue;
			}

			const int32 RemovedQuantity = FMath::Min(StashSlot.Quantity, QuantityToRemove);
			StashSlot.Quantity -= RemovedQuantity;
			QuantityToRemove -= RemovedQuantity;

			if (StashSlot.Quantity <= 0)
			{
				StashSlot.Clear();
			}

			if (QuantityToRemove <= 0)
			{
				break;
			}
		}
	}

	ForceNetUpdate();
}

void APDPlayerState::ClearRaidLoadout()
{
	PersistentData.RaidLoadout.Empty();
	PersistentData.RaidGold = 0;
	ForceNetUpdate();
}

void APDPlayerState::SetPersistentStashSnapshot(const TArray<FPDInventorySlot>& InStashItems, int32 InUpgradeLevel)
{
	PersistentData.StashItems = InStashItems;
	FPDItemContainerOps::EnsureInstanceIDs(PersistentData.StashItems);
	PersistentData.StashUpgradeLevel = FMath::Max(0, InUpgradeLevel);
	ForceNetUpdate();
}

int32 APDPlayerState::CountPersistentStashItems(FName ItemID, bool bCountAnyItem) const
{
	if (ItemID.IsNone())
	{
		return 0;
	}

	int32 TotalQuantity = 0;
	for (const FPDInventorySlot& Slot : PersistentData.StashItems)
	{
		if (!Slot.IsEmpty() && (bCountAnyItem || Slot.ItemData.ItemID == ItemID))
		{
			TotalQuantity += Slot.Quantity;
		}
	}
	return TotalQuantity;
}

int32 APDPlayerState::RemovePersistentStashItems(FName ItemID, int32 Quantity, bool bRemoveAnyItem)
{
	if (ItemID.IsNone() || Quantity <= 0)
	{
		return 0;
	}

	int32 RemainingQuantity = Quantity;
	int32 RemovedQuantity = 0;
	for (int32 Index = PersistentData.StashItems.Num() - 1; Index >= 0 && RemainingQuantity > 0; --Index)
	{
		FPDInventorySlot& Slot = PersistentData.StashItems[Index];
		if (Slot.IsEmpty() || (!bRemoveAnyItem && Slot.ItemData.ItemID != ItemID))
		{
			continue;
		}

		const int32 RemoveAmount = FMath::Min(RemainingQuantity, Slot.Quantity);
		Slot.Quantity -= RemoveAmount;
		RemainingQuantity -= RemoveAmount;
		RemovedQuantity += RemoveAmount;

		if (Slot.Quantity <= 0)
		{
			Slot.Clear();
		}
	}

	if (RemovedQuantity > 0)
	{
		ForceNetUpdate();
	}

	return RemovedQuantity;
}

void APDPlayerState::TransferInventoryToPersistentStash(const UPDInventoryComponent* SourceInventory)
{
	if (!SourceInventory)
	{
		return;
	}

	for (const FPDInventorySlot& RaidSlot : SourceInventory->Items)
	{
		if (RaidSlot.IsEmpty())
		{
			continue;
		}

		int32 RemainingQuantity = RaidSlot.Quantity;

		if (RaidSlot.ItemData.MaxStack > 1)
		{
			for (FPDInventorySlot& StashSlot : PersistentData.StashItems)
			{
				if (StashSlot.IsEmpty() || StashSlot.ItemData.ItemID != RaidSlot.ItemData.ItemID)
				{
					continue;
				}

				const int32 StackSpace = StashSlot.ItemData.MaxStack - StashSlot.Quantity;
				if (StackSpace <= 0)
				{
					continue;
				}

				const int32 AddedQuantity = FMath::Min(StackSpace, RemainingQuantity);
				StashSlot.Quantity += AddedQuantity;
				StashSlot.bIsEmpty = false;
				RemainingQuantity -= AddedQuantity;

				if (RemainingQuantity <= 0)
				{
					break;
				}
			}
		}

		while (RemainingQuantity > 0)
		{
			FPDInventorySlot NewSlot;
			NewSlot.ItemInstanceID = RaidSlot.ItemInstanceID;
			NewSlot.ItemData = RaidSlot.ItemData;
			NewSlot.ModificationLevel = RaidSlot.ModificationLevel;
			NewSlot.Quantity = FMath::Min(RemainingQuantity, FMath::Max(1, RaidSlot.ItemData.MaxStack));
			NewSlot.bIsEmpty = false;
			NewSlot.EnsureInstanceID();

			PersistentData.StashItems.Add(NewSlot);
			RemainingQuantity -= NewSlot.Quantity;
		}
	}

	PersistentData.Gold += SourceInventory->Gold;
	ForceNetUpdate();
}

void APDPlayerState::CaptureEquippedItemsToPersistent()
{
	if (!HasAuthority()) return;

	PersistentData.EquippedItems.Reset();

	const UPDEquipmentComponent* Equip = GetEquipmentComponent();
	if (!Equip) return;

	for (const TPair<EPDEquipmentSlotType, FPDEquippedItem>& Pair : Equip->GetEquippedItems())
	{
		if (!Pair.Value.IsEmpty())
		{
			PersistentData.EquippedItems.Add(Pair.Value);
		}
	}
}

void APDPlayerState::RestoreEquippedItemsFromPersistent()
{
	if (!HasAuthority()) return;
	if (PersistentData.EquippedItems.Num() == 0) return;

	UPDInventoryComponent* Inventory = GetInventoryComponent();
	UPDEquipmentComponent* Equipment = GetEquipmentComponent();
	if (!Inventory || !Equipment) return;

	// 임시 배치용 빈 슬롯 확보를 위해 인벤토리 그리드 보장.
	if (Inventory->Items.Num() != Inventory->GetMaxSlotCount())
	{
		Inventory->InitializeInventory();
	}

	for (const FPDEquippedItem& Saved : PersistentData.EquippedItems)
	{
		if (Saved.IsEmpty()) continue;

		// 이미 같은 슬롯에 장착돼 있으면 중복 복원 방지 (idempotent — 여러 훅에서 호출될 수 있음).
		if (Equipment->IsSlotOccupied(Saved.SlotType)) continue;

		const int32 SlotIndex = Inventory->FindEmptySlot();
		if (SlotIndex == INDEX_NONE) break;

		// 정문 재사용: 저장 아이템을 인벤토리 빈 칸에 임시 배치 후 평소 장착 경로 호출.
		// EquipItemFromInventoryToSlot 이 부수효과/퀵슬롯 참조/리플리케이션을 모두 처리하고 인벤토리에서 빼감.
		Inventory->Items[SlotIndex] = Saved.ItemSlot;
		Inventory->Items[SlotIndex].Quantity = 1;
		Inventory->Items[SlotIndex].bIsEmpty = false;
		Inventory->Items[SlotIndex].EnsureInstanceID();

		// 장착 실패 시 임시 배치 슬롯 정리.
		if (!Equipment->EquipItemFromInventoryToSlot(Inventory, SlotIndex, Saved.SlotType))
		{
			Inventory->Items[SlotIndex].Clear();
		}
	}
}

void APDPlayerState::CaptureQuickSlotItemsToPersistent()
{
	if (!HasAuthority()) return;

	PersistentData.QuickSlotKeptItems.Reset();

	UPDQuickSlotComponent* QuickSlot = GetQuickSlotComponent();
	const UPDInventoryComponent* Inventory = GetInventoryComponent();
	if (!QuickSlot || !Inventory) return;

	// 인덱스 정렬 보존: 슬롯 번호 = 배열 인덱스.
	PersistentData.QuickSlotKeptItems.SetNum(QuickSlot->QuickSlotItems.Num());

	for (int32 i = 0; i < QuickSlot->QuickSlotItems.Num(); ++i)
	{
		const FPDQuickSlotReference& Ref = QuickSlot->QuickSlotItems[i];
		if (Ref.IsEmpty()) continue;

		// 1순위: InstanceID 매칭 (무기 등 안정적 인스턴스).
		const FPDInventorySlot* InvSlot = Inventory->FindSlotByInstanceID(Ref.ItemInstanceID);

		// 2순위: ItemID 폴백 (소모품 — 스택 연산으로 ref 의 InstanceID 가 어긋날 수 있어 ResolveQuickSlotData 도 ItemID 폴백 사용).
		if (!InvSlot || InvSlot->IsEmpty())
		{
			for (const FPDInventorySlot& Candidate : Inventory->Items)
			{
				if (!Candidate.IsEmpty() && Candidate.ItemData.ItemID == Ref.ItemID)
				{
					InvSlot = &Candidate;
					break;
				}
			}
		}

		// 인벤토리에서 못 찾으면 장착 무기를 참조하는 슬롯 → 제외(장비 복원이 처리).
		if (!InvSlot || InvSlot->IsEmpty())
		{
			continue;
		}

		PersistentData.QuickSlotKeptItems[i] = *InvSlot;
	}
}

void APDPlayerState::BuildStashTransferItems(TArray<FPDInventorySlot>& OutItems) const
{
	OutItems.Reset();

	const UPDInventoryComponent* Inventory = GetInventoryComponent();
	if (!Inventory) return;

	// 퀵슬롯 보관 항목의 InstanceID 집합 → 스태시에서 제외. (CaptureQuickSlotItemsToPersistent 먼저 호출돼 있어야 함)
	TSet<FGuid> KeptIDs;
	for (const FPDInventorySlot& Kept : PersistentData.QuickSlotKeptItems)
	{
		if (!Kept.IsEmpty() && Kept.ItemInstanceID.IsValid())
		{
			KeptIDs.Add(Kept.ItemInstanceID);
		}
	}

	for (const FPDInventorySlot& Item : Inventory->Items)
	{
		if (Item.IsEmpty()) continue;
		if (Item.ItemInstanceID.IsValid() && KeptIDs.Contains(Item.ItemInstanceID)) continue;
		OutItems.Add(Item);
	}
}

void APDPlayerState::RestoreQuickSlotItemsFromPersistent()
{
	if (!HasAuthority()) return;
	if (PersistentData.QuickSlotKeptItems.Num() == 0) return;

	UPDQuickSlotComponent* QuickSlot = GetQuickSlotComponent();
	UPDInventoryComponent* Inventory = GetInventoryComponent();
	if (!QuickSlot || !Inventory) return;

	if (Inventory->Items.Num() != Inventory->GetMaxSlotCount())
	{
		Inventory->InitializeInventory();
	}

	for (int32 i = 0; i < PersistentData.QuickSlotKeptItems.Num(); ++i)
	{
		const FPDInventorySlot& Saved = PersistentData.QuickSlotKeptItems[i];
		if (Saved.IsEmpty()) continue;

		// 이미 인벤토리에 복원돼 있으면 중복 방지 (여러 훅에서 호출될 수 있음 — idempotent).
		if (Saved.ItemInstanceID.IsValid() && Inventory->FindSlotIndexByInstanceID(Saved.ItemInstanceID) != INDEX_NONE)
		{
			continue;
		}

		const int32 InvIndex = Inventory->FindEmptySlot();
		if (InvIndex == INDEX_NONE) break;

		Inventory->Items[InvIndex] = Saved;
		Inventory->Items[InvIndex].bIsEmpty = false;
		Inventory->Items[InvIndex].EnsureInstanceID();

		// 퀵슬롯 슬롯 i 에 참조 재설정 (아이템은 인벤토리에 남음 = 원래 모델).
		QuickSlot->StoreInventorySlotQuantityToSlot(Inventory, InvIndex, i, Inventory->Items[InvIndex].Quantity);
	}
}
