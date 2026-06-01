#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Type/Types.h"
#include "PDQuickSlotComponent.generated.h"

class UPDInventoryComponent;
class UPDStashComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPDOnQuickSlotsChanged);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTD_API UPDQuickSlotComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPDQuickSlotComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|QuickSlot")
	TArray<FPDInventorySlot> QuickSlotItems;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|QuickSlot")
	int32 GridColumns = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|QuickSlot")
	int32 GridRows = 1;

	UPROPERTY(BlueprintAssignable, Category="PD|QuickSlot")
	FPDOnQuickSlotsChanged OnQuickSlotsChanged;

	UFUNCTION(BlueprintPure, Category="PD|QuickSlot")
	int32 GetMaxSlotCount() const { return GridColumns * GridRows; }

	UFUNCTION(BlueprintPure, Category="PD|QuickSlot")
	int32 FindEmptySlot() const;

	UFUNCTION(BlueprintCallable, Category="PD|QuickSlot")
	void InitializeQuickSlots();

	UFUNCTION(BlueprintCallable, Category="PD|QuickSlot")
	void ResetQuickSlots();

	UFUNCTION(BlueprintCallable, Category="PD|QuickSlot")
	int32 AddItemPartial(const FPDItemData& ItemData, int32 Quantity = 1);

	UFUNCTION(BlueprintCallable, Category="PD|QuickSlot")
	int32 AddItemToSlotPartial(const FPDItemData& ItemData, int32 Quantity, int32 TargetSlotIndex);

	UFUNCTION(BlueprintCallable, Category="PD|QuickSlot")
	bool MoveSlotQuantityToSlot(int32 SourceSlotIndex, int32 TargetSlotIndex, int32 Quantity);

	UFUNCTION(BlueprintCallable, Category="PD|QuickSlot")
	bool StoreInventorySlotQuantityToSlot(UPDInventoryComponent* SourceInventory, int32 SourceSlotIndex, int32 TargetQuickSlotIndex, int32 Quantity);

	UFUNCTION(BlueprintCallable, Category="PD|QuickSlot")
	bool StoreStashSlotQuantityToSlot(UPDStashComponent* SourceStash, int32 SourceStashSlotIndex, int32 TargetQuickSlotIndex, int32 Quantity);

	UFUNCTION(BlueprintCallable, Category="PD|QuickSlot")
	bool TakeQuickSlotQuantityToInventorySlot(UPDInventoryComponent* TargetInventory, int32 QuickSlotIndex, int32 TargetInventorySlotIndex, int32 Quantity);

	UFUNCTION(BlueprintCallable, Category="PD|QuickSlot")
	bool TakeQuickSlotQuantityToStashSlot(UPDStashComponent* TargetStash, int32 QuickSlotIndex, int32 TargetStashSlotIndex, int32 Quantity);

	UFUNCTION(BlueprintCallable, Category="PD|QuickSlot")
	bool RemoveItemFromSlot(int32 SlotIndex, int32 Quantity = 1);

	UFUNCTION(BlueprintCallable, Category="PD|QuickSlot")
	bool HasItem(FName ItemID, int32 Quantity = 1) const;

protected:
	virtual void BeginPlay() override;
};
