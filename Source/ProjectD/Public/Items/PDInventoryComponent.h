#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Type/Types.h"
#include "PDInventoryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPDOnInventoryChanged);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROJECTD_API UPDInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPDInventoryComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Inventory")
	TArray<FPDInventorySlot> Items;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Inventory")
	int32 GridColumns = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Inventory")
	int32 GridRows = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Inventory")
	int32 Gold = 0;

	UPROPERTY(BlueprintAssignable, Category = "PD|Inventory")
	FPDOnInventoryChanged OnInventoryChanged;

	UFUNCTION(BlueprintCallable, Category = "PD|Inventory")
	bool AddItem(const FPDItemData& ItemData, int32 Quantity = 1);

	UFUNCTION(BlueprintCallable, Category = "PD|Inventory")
	bool RemoveItem(FName ItemID, int32 Quantity = 1);

	UFUNCTION(BlueprintCallable, Category = "PD|Inventory")
	bool RemoveItemFromSlot(int32 SlotIndex, int32 Quantity = 1);

	UFUNCTION(BlueprintCallable, Category = "PD|Inventory")
	bool DropItemFromSlot(int32 SlotIndex, int32 Quantity = 1);

	UFUNCTION(BlueprintCallable, Category = "PD|Inventory")
	bool UseItemFromSlot(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "PD|Inventory")
	bool HasItem(FName ItemID, int32 Quantity = 1) const;

	UFUNCTION(BlueprintCallable, Category = "PD|Inventory")
	void AddGold(int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "PD|Inventory")
	bool SpendGold(int32 Amount);

	UFUNCTION(BlueprintPure, Category = "PD|Inventory")
	int32 GetGold() const { return Gold; }

	UFUNCTION(BlueprintPure, Category = "PD|Inventory")
	int32 GetMaxSlotCount() const { return GridColumns * GridRows; }

	UFUNCTION(BlueprintPure, Category = "PD|Inventory")
	int32 FindEmptySlot() const;

	UFUNCTION(BlueprintCallable, Category = "PD|Inventory")
	int32 AddItemToSlotPartial(const FPDItemData& ItemData, int32 Quantity, int32 TargetSlotIndex);

	UFUNCTION(BlueprintCallable, Category = "PD|Inventory")
	bool MoveSlotQuantityToSlot(int32 SourceSlotIndex, int32 TargetSlotIndex, int32 Quantity);

	UFUNCTION(BlueprintCallable, Category = "PD|Inventory")
	int32 AddItemPartial(const FPDItemData& ItemData, int32 Quantity = 1);

	UFUNCTION(BlueprintCallable, Category = "PD|Inventory")
	void InitializeInventory();

	UFUNCTION(BlueprintCallable, Category = "PD|Inventory")
	void ResetInventory();

protected:
	virtual void BeginPlay() override;
};