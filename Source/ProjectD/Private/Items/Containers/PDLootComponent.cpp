#include "Items/Containers/PDLootComponent.h"

#include "Core/PDPlayerController.h"
#include "Engine/DataTable.h"
#include "GameFramework/Actor.h"
#include "Items/Containers/PDInventoryComponent.h"
#include "Items/Data/PDItemSlotTransfer.h"
#include "Net/UnrealNetwork.h"

namespace
{
	APDPlayerController* GetLootBoxLocalPlayerController(const UObject* WorldContext)
	{
		const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
		return World ? Cast<APDPlayerController>(World->GetFirstPlayerController()) : nullptr;
	}

	// 변경 즉시 클라에 반영되도록 양쪽 컨테이너 소유 액터의 net 갱신 강제.
	void ForceLootReplication(const UPDLootComponent* LootComponent, const UPDInventoryComponent* InventoryComponent)
	{
		if (AActor* LootOwner = LootComponent ? LootComponent->GetOwner() : nullptr)
		{
			LootOwner->FlushNetDormancy();
			LootOwner->ForceNetUpdate();
		}
		if (AActor* InventoryOwner = InventoryComponent ? InventoryComponent->GetOwner() : nullptr)
		{
			InventoryOwner->FlushNetDormancy();
			InventoryOwner->ForceNetUpdate();
		}
	}
}

UPDLootComponent::UPDLootComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UPDLootComponent::BeginPlay()
{
	Super::BeginPlay();
	EnsureSlotCount();
}

void UPDLootComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// COND_None: 모든 ?��?버에�??�기????LootBox ???�구???�일??콘텐츠�? 봐야 ??
	DOREPLIFETIME_CONDITION(UPDLootComponent, LootItems, COND_None);
}

void UPDLootComponent::OnRep_LootItems()
{
	OnLootChanged.Broadcast();
}

void UPDLootComponent::EnsureSlotCount()
{
	const int32 Required = GetMaxSlotCount();
	if (LootItems.Num() < Required)
	{
		LootItems.SetNum(Required);
	}
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		FPDItemContainerOps::EnsureInstanceIDs(LootItems);
	}
}

int32 UPDLootComponent::FindEmptySlot() const
{
	return FPDItemContainerOps::FindEmptySlot(LootItems);
}

bool UPDLootComponent::AddItemByID(FName ItemID, int32 Quantity)
{
	if (ItemID.IsNone() || Quantity <= 0) return false;

	if (!ItemDataTable)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[%s] PDLootComponent: ItemDataTable 미설?? BP_PDLootBoxActor ??LootComponent ?�테?�에??DT_ItemData 지???�요."),
			*GetNameSafe(GetOwner()));
		return false;
	}

	// RowName ???�닌 row ?��? ItemID ?�드�?검????Stash ?� ?�일 ?�책.
	// DT ?�자?�너 ?�영??RowName ?� ?�의값이�?ItemID ?�드가 canonical key ??
	TArray<FPDItemData*> Rows;
	ItemDataTable->GetAllRows<FPDItemData>(TEXT("PDLootComponent::AddItemByID"), Rows);

	for (const FPDItemData* Row : Rows)
	{
		if (Row && Row->ItemID == ItemID)
		{
			return AddItemPartial(*Row, Quantity) > 0;
		}
	}

	UE_LOG(LogTemp, Warning,
		TEXT("[%s] PDLootComponent: ItemDataTable ??ItemID ?�드 '%s' ?�치?�는 row ?�음. DT_ItemData ??ItemID 컬럼 ?�인."),
		*GetNameSafe(GetOwner()), *ItemID.ToString());
	return false;
}

int32 UPDLootComponent::AddItemPartial(const FPDItemData& ItemData, int32 Quantity)
{
	if (Quantity <= 0) return 0;
	EnsureSlotCount();

	int32 TotalAdded = FPDItemContainerOps::AddItem(LootItems, ItemData, Quantity);

	// 가변 그리드: 다 못 들어가면 한 행씩 늘려 나머지를 마저 채움. 진전 없으면 중단(무한 확장 방지).
	while (bAutoSizeRows && TotalAdded < Quantity)
	{
		const int32 Cols = FMath::Max(1, GridColumns);
		LootItems.AddDefaulted(Cols);
		if (GetOwner() && GetOwner()->HasAuthority())
		{
			FPDItemContainerOps::EnsureInstanceIDs(LootItems);
		}

		const int32 AddedMore = FPDItemContainerOps::AddItem(LootItems, ItemData, Quantity - TotalAdded);
		if (AddedMore <= 0) break;
		TotalAdded += AddedMore;
	}

	if (TotalAdded > 0)
	{
		OnLootChanged.Broadcast();
	}
	return TotalAdded;
}

bool UPDLootComponent::TakeSlotToInventory(int32 LootSlotIndex, UPDInventoryComponent* TargetInventory, int32 Quantity)
{
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		if (APDPlayerController* PC = GetLootBoxLocalPlayerController(this))
		{
			PC->ServerTakeLootSlotToInventory(this, LootSlotIndex, Quantity);
			return true;
		}
		return false;
	}

	if (!LootItems.IsValidIndex(LootSlotIndex) || !TargetInventory) return false;

	FPDInventorySlot& Slot = LootItems[LootSlotIndex];
	if (Slot.IsEmpty()) return false;

	const int32 RequestQty = (Quantity < 0) ? Slot.Quantity : FMath::Min(Quantity, Slot.Quantity);
	if (RequestQty <= 0) return false;

	FPDInventorySlot TransferSlot = Slot;
	TransferSlot.Quantity = RequestQty;

	const int32 Added = TargetInventory->AddSlotPartial(TransferSlot);
	if (Added <= 0) return false;

	Slot.Quantity -= Added;
	if (Slot.Quantity <= 0)
	{
		Slot.Clear();
	}

	OnLootChanged.Broadcast();
	TargetInventory->OnInventoryChanged.Broadcast();
	ForceLootReplication(this, TargetInventory);
	return true;
}

bool UPDLootComponent::TakeSlotQuantityToInventorySlot(UPDInventoryComponent* TargetInventory, int32 LootSlotIndex, int32 TargetInventorySlotIndex, int32 Quantity)
{
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		if (APDPlayerController* PC = GetLootBoxLocalPlayerController(this))
		{
			PC->ServerTakeLootSlotQuantityToInventorySlot(this, LootSlotIndex, TargetInventorySlotIndex, Quantity);
			return true;
		}
		return false;
	}

	EnsureSlotCount();
	if (!TargetInventory || !LootItems.IsValidIndex(LootSlotIndex)
		|| !TargetInventory->Items.IsValidIndex(TargetInventorySlotIndex) || Quantity <= 0)
	{
		return false;
	}

	// 인벤토리 무게 한계 검사 — 박스→인벤토리 방향만.
	const FPDInventorySlot& SourceSlot = LootItems[LootSlotIndex];
	if (!TargetInventory->CanAddSlotWeight(SourceSlot, Quantity))
	{
		TargetInventory->BroadcastWeightLimitExceeded();
		return false;
	}

	const bool bMoved = FPDItemSlotTransfer::MoveQuantity(LootItems[LootSlotIndex], TargetInventory->Items[TargetInventorySlotIndex], Quantity);
	if (bMoved)
	{
		OnLootChanged.Broadcast();
		TargetInventory->OnInventoryChanged.Broadcast();
		ForceLootReplication(this, TargetInventory);
	}
	return bMoved;
}

bool UPDLootComponent::StoreInventorySlotQuantityToSlot(UPDInventoryComponent* SourceInventory, int32 SourceSlotIndex, int32 TargetLootSlotIndex, int32 Quantity)
{
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		if (APDPlayerController* PC = GetLootBoxLocalPlayerController(this))
		{
			PC->ServerStoreInventorySlotQuantityToLoot(this, SourceSlotIndex, TargetLootSlotIndex, Quantity);
			return true;
		}
		return false;
	}

	EnsureSlotCount();
	if (!SourceInventory || !SourceInventory->Items.IsValidIndex(SourceSlotIndex)
		|| !LootItems.IsValidIndex(TargetLootSlotIndex) || Quantity <= 0)
	{
		return false;
	}

	// 박스는 무게/용량 무제한 — 무게 검사 없음.
	const bool bMoved = FPDItemSlotTransfer::MoveQuantity(SourceInventory->Items[SourceSlotIndex], LootItems[TargetLootSlotIndex], Quantity);
	if (bMoved)
	{
		SourceInventory->OnInventoryChanged.Broadcast();
		OnLootChanged.Broadcast();
		ForceLootReplication(this, SourceInventory);
	}
	return bMoved;
}

bool UPDLootComponent::MoveSlotQuantityToSlot(int32 SourceSlotIndex, int32 TargetSlotIndex, int32 Quantity)
{
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		if (APDPlayerController* PC = GetLootBoxLocalPlayerController(this))
		{
			PC->ServerMoveLootSlotQuantity(this, SourceSlotIndex, TargetSlotIndex, Quantity);
			return true;
		}
		return false;
	}

	EnsureSlotCount();
	if (!LootItems.IsValidIndex(SourceSlotIndex) || !LootItems.IsValidIndex(TargetSlotIndex)
		|| SourceSlotIndex == TargetSlotIndex || Quantity <= 0)
	{
		return false;
	}

	const bool bMoved = FPDItemSlotTransfer::MoveQuantity(LootItems[SourceSlotIndex], LootItems[TargetSlotIndex], Quantity);
	if (bMoved)
	{
		OnLootChanged.Broadcast();
		ForceLootReplication(this, nullptr);
	}
	return bMoved;
}
