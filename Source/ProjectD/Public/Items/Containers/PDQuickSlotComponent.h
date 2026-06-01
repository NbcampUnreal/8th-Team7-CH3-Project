#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Type/Types.h"
#include "PDQuickSlotComponent.generated.h"

class UPDInventoryComponent;
class UPDEquipmentComponent;
class UAudioComponent;
class APDPlayerCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPDOnQuickSlotsChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPDOnQuickSlotSelectionChanged, int32, NewIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FPDOnConsumableUseStarted, int32, SlotIndex, FPDItemData, ItemData, float, Duration);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPDOnConsumableUseCanceled, int32, SlotIndex, FPDItemData, ItemData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPDOnConsumableUseCompleted, int32, SlotIndex, FPDItemData, ItemData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FPDOnWeaponQuickSlotCooldownStarted, int32, SlotIndex, float, Duration, float, EndTime);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPDOnWeaponQuickSlotCooldownFinished, int32, SlotIndex);

USTRUCT(BlueprintType)
struct FPDQuickSlotReference
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ItemID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGuid ItemInstanceID;

	bool IsEmpty() const
	{
		return ItemID.IsNone() || !ItemInstanceID.IsValid();
	}

	void Clear()
	{
		ItemID = NAME_None;
		ItemInstanceID = FGuid();
	}

	void SetFromSlot(const FPDInventorySlot& SourceSlot)
	{
		if (SourceSlot.IsEmpty() || !SourceSlot.ItemInstanceID.IsValid())
		{
			Clear();
			return;
		}

		ItemID = SourceSlot.ItemData.ItemID;
		ItemInstanceID = SourceSlot.ItemInstanceID;
	}
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTD_API UPDQuickSlotComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPDQuickSlotComponent();

	UPROPERTY(ReplicatedUsing=OnRep_QuickSlotItems, EditAnywhere, BlueprintReadWrite, Category="PD|QuickSlot")
	TArray<FPDQuickSlotReference> QuickSlotItems;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|QuickSlot")
	int32 GridColumns = 6;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|QuickSlot")
	int32 GridRows = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|QuickSlot", meta=(ClampMin="0"))
	int32 WeaponSlotCount = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|QuickSlot", meta=(ClampMin="0.0"))
	float WeaponSwitchCooldown = 3.f;

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

	UFUNCTION(BlueprintPure, Category="PD|QuickSlot")
	bool GetQuickSlotDisplayData(int32 SlotIndex, FPDInventorySlot& OutSlotData) const;

	const FPDInventorySlot* GetResolvedQuickSlot(int32 SlotIndex) const;

	UFUNCTION(BlueprintCallable, Category="PD|QuickSlot")
	void InitializeQuickSlots();

	UFUNCTION(BlueprintCallable, Category="PD|QuickSlot")
	void ResetQuickSlots();

	bool AddSlotReference(const FPDInventorySlot& SourceSlot);

	UFUNCTION(BlueprintCallable, Category="PD|QuickSlot")
	bool MoveSlotQuantityToSlot(int32 SourceSlotIndex, int32 TargetSlotIndex, int32 Quantity);

	UFUNCTION(BlueprintCallable, Category="PD|QuickSlot")
	bool StoreInventorySlotQuantityToSlot(UPDInventoryComponent* SourceInventory, int32 SourceSlotIndex, int32 TargetQuickSlotIndex, int32 Quantity);

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

	void NotifyConsumableAbilityCompleted(int32 SlotIndex, const FPDItemData& ItemData);
	void NotifyConsumableAbilityCanceled(int32 SlotIndex, const FPDItemData& ItemData);

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
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UFUNCTION()
	void HandleInventoryChanged();

	UFUNCTION()
	void HandleEquipmentChanged();

	UFUNCTION()
	void OnRep_QuickSlotItems();

	UFUNCTION()
	void OnRep_SelectedIndex();

	UFUNCTION()
	void OnRep_ConsumableUseState();

	UFUNCTION()
	void OnRep_WeaponCooldownState();

	UFUNCTION(Server, Reliable)
	void ServerSetSelectedIndex(int32 NewIndex);

	UFUNCTION(Server, Reliable)
	void ServerResetQuickSlots();

	UFUNCTION(Server, Reliable)
	void ServerMoveSlotQuantityToSlot(int32 SourceSlotIndex, int32 TargetSlotIndex, int32 Quantity);

	UFUNCTION(Server, Reliable)
	void ServerStoreInventorySlotQuantityToSlot(UPDInventoryComponent* SourceInventory, int32 SourceSlotIndex, int32 TargetQuickSlotIndex, int32 Quantity);

	UFUNCTION(Server, Reliable)
	void ServerRemoveItemFromSlot(int32 SlotIndex, int32 Quantity);

	UFUNCTION(Server, Reliable)
	void ServerUseQuickSlot(int32 SlotIndex);

	UFUNCTION(Server, Reliable)
	void ServerUseInventoryConsumableSlot(int32 InventorySlotIndex);

	UFUNCTION(Server, Reliable)
	void ServerEquipInventoryWeaponSlot(int32 InventorySlotIndex);

	UFUNCTION(Server, Reliable)
	void ServerCancelConsumableUse();

	UPDInventoryComponent* FindOwnerInventory() const;
	UPDEquipmentComponent* FindOwnerEquipment() const;
	APDPlayerCharacter* FindOwnerPlayerCharacter() const;
	int32 GetInventoryItemQuantity(FName ItemID) const;
	int32 GetAvailableItemQuantity(const FPDItemData& ItemData) const;
	int32 FindInventorySlotByInstanceID(const FGuid& ItemInstanceID) const;
	int32 FindEmptySlotForItem(const FPDItemData& ItemData) const;
	int32 FindInventorySlotByItemID(FName ItemID) const;
	int32 FindWeaponQuickSlotByInstanceID(const FGuid& ItemInstanceID) const;
	int32 FindWeaponQuickSlotByItemID(FName ItemID) const;
	bool IsEquippedItem(const FPDItemData& ItemData) const;
	bool SyncQuickSlotsWithInventory();
	FPDInventorySlot ResolveQuickSlotData(const FPDQuickSlotReference& SlotRef) const;
	FPDInventorySlot ResolvePendingConsumableSlot() const;
	bool UseWeaponQuickSlot(int32 SlotIndex, const FPDQuickSlotReference& SlotRef, const FPDInventorySlot& SlotData);
	bool EquipWeaponFromInventorySlot(int32 InventorySlotIndex, int32 CooldownSlotIndex, bool bRespectWeaponSwitchCooldown = true);
	bool BeginConsumableUse(int32 SlotIndex, const FPDQuickSlotReference& SlotRef);
	bool BeginConsumableUse(int32 SlotIndex, const FPDInventorySlot& Slot);
	bool SendConsumableUseGameplayEvent(int32 SlotIndex, const FPDInventorySlot& Slot);
	bool SendConsumableCancelGameplayEvent();
	void StartConsumableUseSound(const FPDItemData& ItemData);
	void StopConsumableUseSound();
	void ForceQuickSlotReplication() const;
	void StartWeaponQuickSlotCooldown(int32 SlotIndex);
	void FinishWeaponQuickSlotCooldown();

	UPROPERTY(ReplicatedUsing=OnRep_SelectedIndex)
	int32 SelectedIndex = INDEX_NONE;

	UPROPERTY(ReplicatedUsing=OnRep_ConsumableUseState)
	bool bIsUsingConsumable = false;

	UPROPERTY(ReplicatedUsing=OnRep_ConsumableUseState)
	int32 UsingConsumableSlotIndex = INDEX_NONE;

	UPROPERTY(ReplicatedUsing=OnRep_ConsumableUseState)
	FName PendingConsumableItemID = NAME_None;

	UPROPERTY(ReplicatedUsing=OnRep_ConsumableUseState)
	FGuid PendingConsumableInstanceID;

	bool bConsumableUsePresentationActive = false;
	int32 PresentedConsumableSlotIndex = INDEX_NONE;

	UPROPERTY(Transient)
	FPDItemData PresentedConsumableItemData;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> ConsumableUseAudioComponent = nullptr;
	UPROPERTY(Replicated)
	float ConsumableUseStartTime = 0.f;

	UPROPERTY(Replicated)
	float ConsumableUseEndTime = 0.f;

	UPROPERTY(Replicated)
	bool bLastConsumableUseCompleted = false;

	UPROPERTY(ReplicatedUsing=OnRep_WeaponCooldownState)
	bool bWeaponQuickSlotCooldownActive = false;

	UPROPERTY(Replicated)
	int32 WeaponCooldownSlotIndex = INDEX_NONE;

	UPROPERTY(Replicated)
	float WeaponCooldownEndTime = 0.f;

	float LocalWeaponCooldownEndTime = 0.f;
	bool bWeaponCooldownPresentationActive = false;
	int32 PresentedWeaponCooldownSlotIndex = INDEX_NONE;
	FTimerHandle WeaponQuickSlotCooldownTimerHandle;

	mutable FPDInventorySlot ResolvedQuickSlotScratch;
};
