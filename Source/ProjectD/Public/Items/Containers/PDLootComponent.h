#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Type/Types.h"
#include "PDLootComponent.generated.h"

class UDataTable;
class UPDInventoryComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPDOnLootChanged);

/**
 * 적 사망 시 스폰되는 LootBox 컨테이너용 컴포넌트.
 *  - 모든 플레이어가 같은 콘텐츠를 봐야 하므로 LootItems 를 전체 옵저버에게 리플리케이션(COND_None).
 *  - Personal Stash 와 책임/구현 모두 분리 — Stash 의 향후 변경에 영향받지 않음.
 *  - 분류/탭/업그레이드/저장 미사용.
 */
UCLASS(ClassGroup = (PD), meta = (BlueprintSpawnableComponent))
class PROJECTD_API UPDLootComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPDLootComponent();

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(EditDefaultsOnly, Category = "PD|Loot")
	TObjectPtr<UDataTable> ItemDataTable;

	// 런타임 박스 내용물(복제). 저작은 LootBox 의 LootTable 로 일원화 — 에디터 직접 편집 차단(표시 전용).
	UPROPERTY(ReplicatedUsing = OnRep_LootItems, VisibleAnywhere, BlueprintReadWrite, Category = "PD|Loot")
	TArray<FPDInventorySlot> LootItems;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Loot", meta = (ClampMin = "1"))
	int32 GridColumns = 5;

	// bAutoSizeRows 가 true 면 GridRows 는 "최소 행 수"로 동작(콘텐츠가 많으면 자동 확장).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Loot", meta = (ClampMin = "1"))
	int32 GridRows = 4;

	/** true 면 아이템 추가 시 행(Row)을 자동 확장 — Columns 고정. 드랍 수가 많아도 잘리지 않고 다 담김(예: 7개 → 2행). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Loot")
	bool bAutoSizeRows = true;

	UPROPERTY(BlueprintAssignable, Category = "PD|Loot")
	FPDOnLootChanged OnLootChanged;

	UFUNCTION(BlueprintPure, Category = "PD|Loot")
	int32 GetMaxSlotCount() const { return GridColumns * GridRows; }

	UFUNCTION(BlueprintPure, Category = "PD|Loot")
	int32 FindEmptySlot() const;

	/** ItemDataTable 의 row 검색 후 추가. Quantity 가 슬롯/스택 한계로 잘리면 잘린 만큼만 들어감. */
	UFUNCTION(BlueprintCallable, Category = "PD|Loot")
	bool AddItemByID(FName ItemID, int32 Quantity = 1);

	/** 직접 ItemData 받아 추가. 성공한 수량 반환. */
	UFUNCTION(BlueprintCallable, Category = "PD|Loot")
	int32 AddItemPartial(const FPDItemData& ItemData, int32 Quantity = 1);

	/** Loot 슬롯의 아이템을 플레이어 인벤토리로 이동. Quantity=-1 이면 전체. (좌클릭 take) */
	UFUNCTION(BlueprintCallable, Category = "PD|Loot")
	bool TakeSlotToInventory(int32 LootSlotIndex, UPDInventoryComponent* TargetInventory, int32 Quantity = -1);

	// ─── 드래그앤드롭 전송 (서버 권위 — 클라 호출 시 PC Server RPC 로 라우팅) ───
	/** Loot → 인벤토리 특정 슬롯(꺼내기). */
	UFUNCTION(BlueprintCallable, Category = "PD|Loot")
	bool TakeSlotQuantityToInventorySlot(UPDInventoryComponent* TargetInventory, int32 LootSlotIndex, int32 TargetInventorySlotIndex, int32 Quantity);

	/** 인벤토리 → Loot 특정 슬롯(박스에 넣기). */
	UFUNCTION(BlueprintCallable, Category = "PD|Loot")
	bool StoreInventorySlotQuantityToSlot(UPDInventoryComponent* SourceInventory, int32 SourceSlotIndex, int32 TargetLootSlotIndex, int32 Quantity);

	/** Loot 내부 슬롯 간 이동/스왑(박스 안 재배치). */
	UFUNCTION(BlueprintCallable, Category = "PD|Loot")
	bool MoveSlotQuantityToSlot(int32 SourceSlotIndex, int32 TargetSlotIndex, int32 Quantity);

protected:
	UFUNCTION()
	void OnRep_LootItems();

	// 그리드 크기에 맞춰 LootItems 배열 정규화.
	void EnsureSlotCount();
};
