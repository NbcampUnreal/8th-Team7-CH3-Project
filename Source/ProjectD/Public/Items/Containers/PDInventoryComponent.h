#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Type/Types.h"
#include "Engine/DataTable.h"
#include "PDInventoryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPDOnInventoryChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPDOnInventoryWeightLimitExceeded, float, CurrentWeight, float, MaxWeight);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPDOnInventoryMessage, const FText&, Message);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROJECTD_API UPDInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPDInventoryComponent();

	UPROPERTY(EditDefaultsOnly, Category="PD|Inventory")
	TObjectPtr<UDataTable> ItemDataTable;

	UPROPERTY(ReplicatedUsing=OnRep_Items, EditAnywhere, BlueprintReadWrite, Category = "PD|Inventory")
	TArray<FPDInventorySlot> Items;

	UPROPERTY(ReplicatedUsing=OnRep_GridConfig, EditAnywhere, BlueprintReadWrite, Category = "PD|Inventory", meta = (ClampMin = "1"))
	int32 GridColumns = 4;

	UPROPERTY(ReplicatedUsing=OnRep_GridConfig, EditAnywhere, BlueprintReadWrite, Category = "PD|Inventory", meta = (ClampMin = "1"))
	int32 GridRows = 4;

	UPROPERTY(ReplicatedUsing=OnRep_Gold, EditAnywhere, BlueprintReadWrite, Category = "PD|Inventory")
	int32 Gold = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Inventory", meta = (ClampMin = "0.0"))
	float BaseCarryWeight = 10.f;

	UPROPERTY(BlueprintAssignable, Category = "PD|Inventory")
	FPDOnInventoryChanged OnInventoryChanged;

	UPROPERTY(BlueprintAssignable, Category = "PD|Inventory")
	FPDOnInventoryWeightLimitExceeded OnInventoryWeightLimitExceeded;

	UPROPERTY(BlueprintAssignable, Category = "PD|Inventory")
	FPDOnInventoryMessage OnInventoryMessage;

	UFUNCTION(BlueprintCallable, Category = "PD|Inventory")
	bool AddItem(const FPDItemData& ItemData, int32 Quantity = 1);

	UFUNCTION(BlueprintCallable, Category="PD|Inventory")
	bool AddItemByID(FName ItemID, int32 Quantity = 1);

	UFUNCTION(BlueprintPure, Category="PD|Inventory")
	bool FindItemDataByID(FName ItemID, FPDItemData& OutItemData) const;

	UFUNCTION(BlueprintCallable, Category = "PD|Inventory")
	bool RemoveItem(FName ItemID, int32 Quantity = 1);

	UFUNCTION(BlueprintCallable, Category = "PD|Inventory")
	bool RemoveItemFromSlot(int32 SlotIndex, int32 Quantity = 1);

	UFUNCTION(BlueprintCallable, Category = "PD|Inventory")
	bool DropItemFromSlot(int32 SlotIndex, int32 Quantity = 1);

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

	UFUNCTION(BlueprintPure, Category = "PD|Inventory|Weight")
	float GetCurrentWeight() const;

	UFUNCTION(BlueprintPure, Category = "PD|Inventory|Weight")
	float GetMaxWeight() const;

	UFUNCTION(BlueprintPure, Category = "PD|Inventory|Weight")
	bool CanAddWeight(const FPDItemData& ItemData, int32 Quantity = 1) const;

	UFUNCTION(BlueprintPure, Category = "PD|Inventory|Weight")
	bool CanAddSlotWeight(const FPDInventorySlot& SourceSlot, int32 Quantity = 1) const;

	UFUNCTION(BlueprintPure, Category = "PD|Inventory|Weight")
	bool CanFitWeightAfterEquipmentChange(const FPDInventorySlot& ItemLeavingInventory, const FPDInventorySlot& ItemEnteringInventory, float NewBagCapacityWeight) const;

	UFUNCTION(BlueprintCallable, Category = "PD|Inventory")
	void BroadcastInventoryMessage(const FText& Message);

	UFUNCTION(BlueprintCallable, Category = "PD|Inventory|Weight")
	void BroadcastWeightLimitExceeded();

	UFUNCTION(BlueprintPure, Category = "PD|Inventory")
	int32 FindEmptySlot() const;

	UFUNCTION(BlueprintPure, Category = "PD|Inventory")
	int32 FindSlotIndexByInstanceID(const FGuid& ItemInstanceID) const;

	const FPDInventorySlot* FindSlotByInstanceID(const FGuid& ItemInstanceID) const;

	UFUNCTION(BlueprintCallable, Category = "PD|Inventory")
	int32 AddItemToSlotPartial(const FPDItemData& ItemData, int32 Quantity, int32 TargetSlotIndex);

	UFUNCTION(BlueprintCallable, Category = "PD|Inventory")
	bool MoveSlotQuantityToSlot(int32 SourceSlotIndex, int32 TargetSlotIndex, int32 Quantity);

	UFUNCTION(BlueprintCallable, Category = "PD|Inventory")
	int32 AddItemPartial(const FPDItemData& ItemData, int32 Quantity = 1);

	UFUNCTION(BlueprintCallable, Category = "PD|Inventory")
	int32 AddSlotPartial(const FPDInventorySlot& SourceSlot);

	UFUNCTION(BlueprintCallable, Category = "PD|Inventory")
	void InitializeInventory();

	UFUNCTION(BlueprintCallable, Category = "PD|Inventory")
	void ResetInventory();

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_Items();

	UFUNCTION()
	void OnRep_Gold();

	UFUNCTION()
	void OnRep_GridConfig();

	UFUNCTION(Server, Reliable)
	void ServerRemoveItemFromSlot(int32 SlotIndex, int32 Quantity);

	UFUNCTION(Server, Reliable)
	void ServerDropItemFromSlot(int32 SlotIndex, int32 Quantity);

	UFUNCTION(Server, Reliable)
	void ServerMoveSlotQuantityToSlot(int32 SourceSlotIndex, int32 TargetSlotIndex, int32 Quantity);

	UFUNCTION(Client, Reliable)
	void ClientBroadcastInventoryMessage(const FText& Message);

	UFUNCTION(Client, Reliable)
	void ClientBroadcastWeightLimitExceeded(float CurrentWeight, float MaxWeight, const FText& Message);
};
