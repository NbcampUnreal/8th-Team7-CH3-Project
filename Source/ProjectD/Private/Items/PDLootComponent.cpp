#include "Items/PDLootComponent.h"

#include "Engine/DataTable.h"
#include "Items/PDInventoryComponent.h"
#include "Net/UnrealNetwork.h"

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

	// COND_None: 모든 옵저버에게 동기화 — LootBox 는 누구나 동일한 콘텐츠를 봐야 함.
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
}

int32 UPDLootComponent::FindEmptySlot() const
{
	for (int32 i = 0; i < LootItems.Num(); ++i)
	{
		if (LootItems[i].IsEmpty()) return i;
	}
	return INDEX_NONE;
}

bool UPDLootComponent::AddItemByID(FName ItemID, int32 Quantity)
{
	if (ItemID.IsNone() || Quantity <= 0) return false;

	if (!ItemDataTable)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[%s] PDLootComponent: ItemDataTable 미설정. BP_PDLootBoxActor 의 LootComponent 디테일에서 DT_ItemData 지정 필요."),
			*GetNameSafe(GetOwner()));
		return false;
	}

	// RowName 이 아닌 row 내부 ItemID 필드로 검색 — Stash 와 동일 정책.
	// DT 디자이너 운영상 RowName 은 임의값이고 ItemID 필드가 canonical key 임.
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
		TEXT("[%s] PDLootComponent: ItemDataTable 에 ItemID 필드 '%s' 일치하는 row 없음. DT_ItemData 의 ItemID 컬럼 확인."),
		*GetNameSafe(GetOwner()), *ItemID.ToString());
	return false;
}

int32 UPDLootComponent::AddItemPartial(const FPDItemData& ItemData, int32 Quantity)
{
	if (Quantity <= 0) return 0;
	EnsureSlotCount();

	int32 Remaining = Quantity;
	const int32 MaxStack = FMath::Max(ItemData.MaxStack, 1);

	// 1) 같은 ItemID 의 기존 스택에 채우기.
	for (int32 i = 0; i < LootItems.Num() && Remaining > 0; ++i)
	{
		FPDInventorySlot& Slot = LootItems[i];
		if (Slot.IsEmpty()) continue;
		if (Slot.ItemData.ItemID != ItemData.ItemID) continue;
		const int32 CanAdd = MaxStack - Slot.Quantity;
		if (CanAdd <= 0) continue;
		const int32 ToAdd = FMath::Min(CanAdd, Remaining);
		Slot.Quantity += ToAdd;
		Slot.bIsEmpty = false;
		Remaining -= ToAdd;
	}

	// 2) 남은 만큼 빈 슬롯에 신규 스택 생성.
	for (int32 i = 0; i < LootItems.Num() && Remaining > 0; ++i)
	{
		FPDInventorySlot& Slot = LootItems[i];
		if (!Slot.IsEmpty()) continue;
		const int32 ToAdd = FMath::Min(MaxStack, Remaining);
		Slot.ItemData = ItemData;
		Slot.Quantity = ToAdd;
		Slot.bIsEmpty = false;
		Remaining -= ToAdd;
	}

	const int32 Added = Quantity - Remaining;
	if (Added > 0)
	{
		OnLootChanged.Broadcast();
	}
	return Added;
}

bool UPDLootComponent::TakeSlotToInventory(int32 LootSlotIndex, UPDInventoryComponent* TargetInventory, int32 Quantity)
{
	if (!LootItems.IsValidIndex(LootSlotIndex) || !TargetInventory) return false;

	FPDInventorySlot& Slot = LootItems[LootSlotIndex];
	if (Slot.IsEmpty()) return false;

	const int32 RequestQty = (Quantity < 0) ? Slot.Quantity : FMath::Min(Quantity, Slot.Quantity);
	if (RequestQty <= 0) return false;

	const int32 Added = TargetInventory->AddItemPartial(Slot.ItemData, RequestQty);
	if (Added <= 0) return false;

	Slot.Quantity -= Added;
	if (Slot.Quantity <= 0)
	{
		Slot.Clear();
	}

	OnLootChanged.Broadcast();
	return true;
}
