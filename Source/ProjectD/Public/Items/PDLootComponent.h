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

	UPROPERTY(ReplicatedUsing = OnRep_LootItems, EditAnywhere, BlueprintReadWrite, Category = "PD|Loot")
	TArray<FPDInventorySlot> LootItems;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Loot", meta = (ClampMin = "1"))
	int32 GridColumns = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Loot", meta = (ClampMin = "1"))
	int32 GridRows = 4;

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

	/** Loot 슬롯의 아이템을 플레이어 인벤토리로 이동. Quantity=-1 이면 전체. */
	UFUNCTION(BlueprintCallable, Category = "PD|Loot")
	bool TakeSlotToInventory(int32 LootSlotIndex, UPDInventoryComponent* TargetInventory, int32 Quantity = -1);

protected:
	UFUNCTION()
	void OnRep_LootItems();

	// 그리드 크기에 맞춰 LootItems 배열 정규화.
	void EnsureSlotCount();
};
