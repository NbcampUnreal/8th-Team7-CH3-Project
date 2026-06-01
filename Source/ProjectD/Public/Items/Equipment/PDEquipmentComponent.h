#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Type/Types.h"
#include "PDEquipmentComponent.generated.h"

class UPDInventoryComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPDOnEquipmentSlotChanged, EPDEquipmentSlotType, SlotType, FPDInventorySlot, EquippedSlot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPDOnEquipmentChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FPDOnEquipmentModificationApplied, EPDEquipmentSlotType, SlotType, FPDInventorySlot, EquippedSlot, int32, GasLevel);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROJECTD_API UPDEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPDEquipmentComponent();

	UPROPERTY(BlueprintAssignable, Category = "PD|Equipment")
	FPDOnEquipmentChanged OnEquipmentChanged;

	UPROPERTY(BlueprintAssignable, Category = "PD|Equipment")
	FPDOnEquipmentSlotChanged OnEquipmentSlotChanged;

	UPROPERTY(BlueprintAssignable, Category = "PD|Equipment|GAS")
	FPDOnEquipmentModificationApplied OnEquipmentModificationApplied;

	UFUNCTION(BlueprintCallable, Category = "PD|Equipment")
	bool EquipItemFromInventory(UPDInventoryComponent* InventoryComponent, int32 InventorySlotIndex);

	UFUNCTION(BlueprintCallable, Category = "PD|Equipment")
	bool EquipItemFromInventoryToSlot(UPDInventoryComponent* InventoryComponent, int32 InventorySlotIndex, EPDEquipmentSlotType TargetSlotType);

	UFUNCTION(BlueprintCallable, Category = "PD|Equipment")
	bool TryEquipNewItem(const FPDItemData& ItemData);

	UFUNCTION(BlueprintCallable, Category = "PD|Equipment")
	bool UnequipItemToInventory(UPDInventoryComponent* InventoryComponent, EPDEquipmentSlotType SlotType);

	UFUNCTION(BlueprintCallable, Category = "PD|Equipment")
	bool UnequipItemToInventorySlot(UPDInventoryComponent* InventoryComponent, EPDEquipmentSlotType SlotType, int32 InventorySlotIndex);

	UFUNCTION(BlueprintPure, Category = "PD|Equipment")
	bool CanEquipItem(const FPDItemData& ItemData) const;

	UFUNCTION(BlueprintPure, Category = "PD|Equipment")
	bool IsSlotOccupied(EPDEquipmentSlotType SlotType) const;

	UFUNCTION(BlueprintPure, Category = "PD|Equipment")
	FPDInventorySlot GetEquippedSlot(EPDEquipmentSlotType SlotType) const;

	UFUNCTION(BlueprintPure, Category = "PD|Equipment")
	EPDEquipmentSlotType ResolveEquipmentSlotType(const FPDItemData& ItemData) const;

	const TMap<EPDEquipmentSlotType, FPDEquippedItem>& GetEquippedItems() const { return EquippedItems; }

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UFUNCTION()
	void OnRep_EquippedItems();

	UFUNCTION(Server, Reliable)
	void ServerEquipItemFromInventory(UPDInventoryComponent* InventoryComponent, int32 InventorySlotIndex);

	UFUNCTION(Server, Reliable)
	void ServerEquipItemFromInventoryToSlot(UPDInventoryComponent* InventoryComponent, int32 InventorySlotIndex, EPDEquipmentSlotType TargetSlotType);

	UFUNCTION(Server, Reliable)
	void ServerUnequipItemToInventory(UPDInventoryComponent* InventoryComponent, EPDEquipmentSlotType SlotType);

	void InitializeDefaultSlots();
	void RebuildEquippedItemsFromReplication();
	void SyncReplicatedEquippedItems();
	bool ApplyCharacterEquipSideEffects(const FPDInventorySlot& ItemSlot) const;
	void RemoveCharacterEquipSideEffects(const FPDInventorySlot& ItemSlot,
	                                      FPDWeaponInstanceState* OutWeaponState = nullptr) const;
	int32 ConvertModificationLevelToGasLevel(int32 ModificationLevel) const;
	void BroadcastModificationApplied(EPDEquipmentSlotType SlotType, const FPDInventorySlot& EquippedSlot);
	void BroadcastSlotChanged(EPDEquipmentSlotType SlotType);
	void BroadcastAllSlotsChanged();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Equipment", meta = (AllowPrivateAccess = "true"))
	TMap<EPDEquipmentSlotType, FPDEquippedItem> EquippedItems;

	UPROPERTY(ReplicatedUsing=OnRep_EquippedItems)
	TArray<FPDEquippedItem> ReplicatedEquippedItems;
};
