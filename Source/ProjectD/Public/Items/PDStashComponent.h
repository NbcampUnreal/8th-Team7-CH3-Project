#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Items/PDInventoryComponent.h"
#include "PDStashComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPDOnStashChanged);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTD_API UPDStashComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPDStashComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|Stash")
	TArray<FPDInventorySlot> StashItems;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|Stash")
	int32 GridColumns = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|Stash")
	int32 GridRows = 8;

	UPROPERTY(BlueprintAssignable, Category="PD|Stash")
	FPDOnStashChanged OnStashChanged;

	UFUNCTION(BlueprintPure, Category="PD|Stash")
	int32 GetMaxSlotCount() const { return GridColumns * GridRows; }

	UFUNCTION(BlueprintPure, Category="PD|Stash")
	int32 FindEmptySlot() const;

	UFUNCTION(BlueprintCallable, Category="PD|Stash")
	void InitializeStash();

	UFUNCTION(BlueprintCallable, Category="PD|Stash")
	void ResetStash();

	UFUNCTION(BlueprintCallable, Category="PD|Stash")
	int32 AddItemPartial(const FPDItemData& ItemData, int32 Quantity = 1);

	UFUNCTION(BlueprintCallable, Category="PD|Stash")
	bool RemoveItem(FName ItemID, int32 Quantity = 1);


	UFUNCTION(BlueprintCallable, Category="PD|Stash")
	bool StoreInventorySlot(UPDInventoryComponent* SourceInventory, int32 SourceSlotIndex);

	UFUNCTION(BlueprintCallable, Category="PD|Stash")
	bool StoreInventorySlotQuantity(UPDInventoryComponent* SourceInventory, int32 SourceSlotIndex, int32 Quantity);

	UFUNCTION(BlueprintCallable, Category="PD|Stash")
	bool TakeStashSlot(UPDInventoryComponent* TargetInventory, int32 StashSlotIndex);

	UFUNCTION(BlueprintCallable, Category="PD|Stash")
	bool TakeStashSlotQuantity(UPDInventoryComponent* TargetInventory, int32 StashSlotIndex, int32 Quantity);

	UFUNCTION(BlueprintCallable, Category="PD|Stash")
	bool HasItem(FName ItemID, int32 Quantity = 1) const;

protected:
	virtual void BeginPlay() override;
};
