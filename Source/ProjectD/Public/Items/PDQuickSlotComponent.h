#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Type/Types.h"
#include "PDQuickSlotComponent.generated.h"

class UPDInventoryComponent;
class UPDEquipmentComponent;
class UPDStashComponent;
class UCharacterMovementComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPDOnQuickSlotsChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPDOnQuickSlotSelectionChanged, int32, NewIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FPDOnConsumableUseStarted, int32, SlotIndex, FPDItemData, ItemData, float, Duration);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPDOnConsumableUseCanceled, int32, SlotIndex, FPDItemData, ItemData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPDOnConsumableUseCompleted, int32, SlotIndex, FPDItemData, ItemData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FPDOnWeaponQuickSlotCooldownStarted, int32, SlotIndex, float, Duration, float, EndTime);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPDOnWeaponQuickSlotCooldownFinished, int32, SlotIndex);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTD_API UPDQuickSlotComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPDQuickSlotComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|QuickSlot")
	TArray<FPDInventorySlot> QuickSlotItems;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|QuickSlot")
	int32 GridColumns = 6;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|QuickSlot")
	int32 GridRows = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|QuickSlot", meta=(ClampMin="0"))
	int32 WeaponSlotCount = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|QuickSlot", meta=(ClampMin="0.0"))
	float WeaponSwitchCooldown = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|QuickSlot", meta=(ClampMin="0.0", ClampMax="1.0"))
	float ConsumableMoveSpeedMultiplier = 0.5f;

	UPROPERTY(BlueprintAssignable, Category="PD|QuickSlot")
	FPDOnQuickSlotsChanged OnQuickSlotsChanged;

	UPROPERTY(BlueprintAssignable, Category="PD|QuickSlot")
	FPDOnQuickSlotSelectionChanged OnSelectionChanged;

	UPROPERTY(BlueprintAssignable, Category="PD|QuickSlot")
	FPDOnConsumableUseStarted OnConsumableUseStarted;

	UPROPERTY(BlueprintAssignable, Category="PD|QuickSlot")
	FPDOnConsumableUseCanceled OnConsumableUseCanceled;

	UPROPERTY(BlueprintAssignable, Category="PD|QuickSlot")
	FPDOnConsumableUseCompleted OnConsumableUseCompleted;

	UPROPERTY(BlueprintAssignable, Category="PD|QuickSlot")
	FPDOnWeaponQuickSlotCooldownStarted OnWeaponQuickSlotCooldownStarted;

	UPROPERTY(BlueprintAssignable, Category="PD|QuickSlot")
	FPDOnWeaponQuickSlotCooldownFinished OnWeaponQuickSlotCooldownFinished;

	UFUNCTION(BlueprintCallable, Category="PD|QuickSlot")
	void SetSelectedIndex(int32 NewIndex);

	UFUNCTION(BlueprintPure, Category="PD|QuickSlot")
	int32 GetSelectedIndex() const { return SelectedIndex; }

	UFUNCTION(BlueprintPure, Category="PD|QuickSlot")
	int32 GetMaxSlotCount() const { return GridColumns * GridRows; }

	UFUNCTION(BlueprintCallable, Category="PD|QuickSlot")
	void SetWeaponSlotCount(int32 NewCount);

	UFUNCTION(BlueprintPure, Category="PD|QuickSlot")
	int32 FindEmptySlot() const;

	UFUNCTION(BlueprintPure, Category="PD|QuickSlot")
	bool IsWeaponQuickSlot(int32 SlotIndex) const;

	UFUNCTION(BlueprintPure, Category="PD|QuickSlot")
	bool IsConsumableQuickSlot(int32 SlotIndex) const;

	UFUNCTION(BlueprintPure, Category="PD|QuickSlot")
	bool CanAssignItemToSlot(const FPDItemData& ItemData, int32 SlotIndex) const;

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
	bool UseQuickSlot(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category="PD|QuickSlot")
	bool UseInventoryConsumableSlot(int32 InventorySlotIndex);

	UFUNCTION(BlueprintCallable, Category="PD|QuickSlot")
	bool EquipInventoryWeaponSlot(int32 InventorySlotIndex);

	UFUNCTION(BlueprintCallable, Category="PD|QuickSlot")
	bool CancelConsumableUse();

	UFUNCTION(BlueprintPure, Category="PD|QuickSlot")
	bool IsUsingConsumable() const { return bIsUsingConsumable; }

	UFUNCTION(BlueprintPure, Category="PD|QuickSlot")
	int32 GetUsingConsumableSlotIndex() const { return UsingConsumableSlotIndex; }

	UFUNCTION(BlueprintPure, Category="PD|QuickSlot")
	float GetConsumableUseRemainingTime() const;

	UFUNCTION(BlueprintPure, Category="PD|QuickSlot")
	float GetConsumableUseProgress() const;

	UFUNCTION(BlueprintPure, Category="PD|QuickSlot")
	bool IsWeaponQuickSlotOnCooldown() const;

	UFUNCTION(BlueprintPure, Category="PD|QuickSlot")
	float GetWeaponQuickSlotCooldownRemainingTime() const;

	UFUNCTION(BlueprintPure, Category="PD|QuickSlot")
	int32 GetWeaponQuickSlotCooldownSlotIndex() const { return WeaponCooldownSlotIndex; }

	UFUNCTION(BlueprintCallable, Category="PD|QuickSlot")
	bool HasItem(FName ItemID, int32 Quantity = 1) const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleInventoryChanged();

	UPDInventoryComponent* FindOwnerInventory() const;
	UPDEquipmentComponent* FindOwnerEquipment() const;
	int32 GetInventoryItemQuantity(FName ItemID) const;
	int32 GetAvailableItemQuantity(const FPDItemData& ItemData) const;
	int32 FindEmptySlotForItem(const FPDItemData& ItemData) const;
	int32 FindInventorySlotByItemID(FName ItemID) const;
	int32 FindWeaponQuickSlotByItemID(FName ItemID) const;
	bool IsEquippedItem(const FPDItemData& ItemData) const;
	bool SyncQuickSlotsWithInventory();
	bool UseWeaponQuickSlot(int32 SlotIndex, const FPDInventorySlot& Slot);
	bool EquipWeaponFromInventorySlot(int32 InventorySlotIndex, int32 CooldownSlotIndex);
	bool BeginConsumableUse(int32 SlotIndex, const FPDInventorySlot& Slot);
	void FinishConsumableUse();
	bool ConsumeItem(const FPDInventorySlot& Slot);
	void ApplyConsumableMoveSpeed();
	void RestoreConsumableMoveSpeed();
	UCharacterMovementComponent* FindOwnerMovementComponent() const;
	void StartWeaponQuickSlotCooldown(int32 SlotIndex);
	void FinishWeaponQuickSlotCooldown();

	int32 SelectedIndex = INDEX_NONE;
	bool bIsUsingConsumable = false;
	int32 UsingConsumableSlotIndex = INDEX_NONE;
	FPDInventorySlot PendingConsumableSlot;
	FTimerHandle ConsumableUseTimerHandle;
	float ConsumableUseStartTime = 0.f;
	float ConsumableUseEndTime = 0.f;
	float CachedMaxWalkSpeed = 0.f;
	bool bMoveSpeedAdjusted = false;
	bool bWeaponQuickSlotCooldownActive = false;
	int32 WeaponCooldownSlotIndex = INDEX_NONE;
	float WeaponCooldownEndTime = 0.f;
	FTimerHandle WeaponQuickSlotCooldownTimerHandle;
};
