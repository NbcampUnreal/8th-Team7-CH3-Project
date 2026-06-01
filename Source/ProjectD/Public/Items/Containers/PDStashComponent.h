#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Items/Containers/PDInventoryComponent.h"
#include "Engine/DataTable.h"
#include "PDStashComponent.generated.h"

UENUM(BlueprintType)
enum class EPDStashUpgradeResult : uint8
{
	Success UMETA(DisplayName = "Success"),
	InvalidInventory UMETA(DisplayName = "Invalid Inventory"),
	AlreadyMaxLevel UMETA(DisplayName = "Already Max Level"),
	NotEnoughGold UMETA(DisplayName = "Not Enough Gold"),
	InvalidConfig UMETA(DisplayName = "Invalid Config")
};

UENUM(BlueprintType)
enum class EPDStashPersistenceMode : uint8
{
	Transient UMETA(DisplayName = "Transient World Stash"),
	PlayerStatePersistent UMETA(DisplayName = "Player State Persistent Save"),
	GameInstanceLegacy UMETA(DisplayName = "Game Instance Legacy Save")
};

USTRUCT(BlueprintType)
struct FPDStashUpgradeData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|Stash")
	int32 Cost = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|Stash")
	int32 AddedRows = 1;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPDOnStashChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPDOnStashUpgradeFailed, EPDStashUpgradeResult, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPDOnStashUpgraded, int32, NewUpgradeLevel, int32, NewGridRows);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTD_API UPDStashComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPDStashComponent();

	UPROPERTY(EditDefaultsOnly, Category="PD|Stash")
	TObjectPtr<UDataTable> ItemDataTable;

	UPROPERTY(ReplicatedUsing=OnRep_StashItems, EditAnywhere, BlueprintReadWrite, Category="PD|Stash")
	TArray<FPDInventorySlot> StashItems;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|Stash")
	int32 GridColumns = 5;

	UPROPERTY(ReplicatedUsing=OnRep_StashConfig, EditAnywhere, BlueprintReadWrite, Category="PD|Stash")
	int32 GridRows = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|Stash")
	int32 BaseGridRows = 4;

	UPROPERTY(ReplicatedUsing=OnRep_StashConfig, EditAnywhere, BlueprintReadWrite, Category="PD|Stash")
	int32 CurrentUpgradeLevel = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|Stash")
	TArray<FPDStashUpgradeData> UpgradeData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|Stash|Persistence")
	EPDStashPersistenceMode PersistenceMode = EPDStashPersistenceMode::Transient;

	UPROPERTY(BlueprintAssignable, Category="PD|Stash")
	FPDOnStashChanged OnStashChanged;

	UPROPERTY(BlueprintAssignable, Category="PD|Stash")
	FPDOnStashUpgraded OnStashUpgraded;

	UPROPERTY(BlueprintAssignable, Category="PD|Stash")
	FPDOnStashUpgradeFailed OnStashUpgradeFailed;

	UFUNCTION(BlueprintPure, Category="PD|Stash")
	int32 GetMaxSlotCount() const { return GridColumns * GridRows; }

	UFUNCTION(BlueprintPure, Category="PD|Stash")
	int32 GetMaxUpgradeLevel() const { return FMath::Min(3, UpgradeData.Num()); }

	UFUNCTION(BlueprintPure, Category="PD|Stash")
	bool IsMaxUpgradeLevel() const { return CurrentUpgradeLevel >= GetMaxUpgradeLevel(); }

	UFUNCTION(BlueprintPure, Category="PD|Stash")
	int32 GetNextUpgradeCost() const;

	UFUNCTION(BlueprintPure, Category="PD|Stash")
	int32 GetNextUpgradeAddedRows() const;

	UFUNCTION(BlueprintPure, Category="PD|Stash")
	EPDStashUpgradeResult CanUpgradeStash(const UPDInventoryComponent* SourceInventory) const;

	UFUNCTION(BlueprintCallable, Category="PD|Stash")
	EPDStashUpgradeResult UpgradeStash(UPDInventoryComponent* SourceInventory);

	UFUNCTION(BlueprintCallable, Category="PD|Stash")
	void SetStashUpgradeLevel(int32 NewUpgradeLevel);

	UFUNCTION(BlueprintPure, Category="PD|Stash")
	int32 FindEmptySlot() const;

	UFUNCTION(BlueprintCallable, Category="PD|Stash")
	void InitializeStash();

	UFUNCTION(BlueprintCallable, Category="PD|Stash")
	void ResetStash();

	UFUNCTION(BlueprintCallable, Category="PD|Stash")
	void LoadFromGameInstance();

	UFUNCTION(BlueprintCallable, Category="PD|Stash")
	void LoadFromPlayerState(class APDPlayerState* PlayerState);

	UFUNCTION(BlueprintCallable, Category="PD|Stash")
	void SaveToGameInstance();

	UFUNCTION(BlueprintCallable, Category="PD|Stash")
	void SaveToPlayerState(class APDPlayerState* PlayerState, bool bSaveToDisk = true);

	UFUNCTION(BlueprintCallable, Category="PD|Stash")
	void PersistIfNeeded();

	void PersistIfNeededForInventory(const UPDInventoryComponent* InventoryComponent);

	UFUNCTION(BlueprintCallable, Category="PD|Stash")
	int32 AddItemPartial(const FPDItemData& ItemData, int32 Quantity = 1);

	UFUNCTION(BlueprintCallable, Category="PD|Stash")
	bool AddItemByID(FName ItemID, int32 Quantity = 1);

	UFUNCTION(BlueprintCallable, Category="PD|Stash")
	int32 AddItemToSlotPartial(const FPDItemData& ItemData, int32 Quantity, int32 TargetSlotIndex);

	UFUNCTION(BlueprintCallable, Category="PD|Stash")
	bool MoveSlotQuantityToSlot(int32 SourceSlotIndex, int32 TargetSlotIndex, int32 Quantity);

	UFUNCTION(BlueprintCallable, Category="PD|Stash")
	bool RemoveItem(FName ItemID, int32 Quantity = 1);


	UFUNCTION(BlueprintCallable, Category="PD|Stash")
	bool StoreInventorySlot(UPDInventoryComponent* SourceInventory, int32 SourceSlotIndex);

	UFUNCTION(BlueprintCallable, Category="PD|Stash")
	bool StoreInventorySlotQuantity(UPDInventoryComponent* SourceInventory, int32 SourceSlotIndex, int32 Quantity);

	UFUNCTION(BlueprintCallable, Category="PD|Stash")
	bool StoreInventorySlotQuantityToSlot(UPDInventoryComponent* SourceInventory, int32 SourceSlotIndex, int32 TargetStashSlotIndex, int32 Quantity);

	UFUNCTION(BlueprintCallable, Category="PD|Stash")
	bool TakeStashSlot(UPDInventoryComponent* TargetInventory, int32 StashSlotIndex);

	UFUNCTION(BlueprintCallable, Category="PD|Stash")
	bool TakeStashSlotQuantity(UPDInventoryComponent* TargetInventory, int32 StashSlotIndex, int32 Quantity);

	UFUNCTION(BlueprintCallable, Category="PD|Stash")
	bool TakeStashSlotQuantityToInventorySlot(UPDInventoryComponent* TargetInventory, int32 StashSlotIndex, int32 TargetInventorySlotIndex, int32 Quantity);

	UFUNCTION(BlueprintCallable, Category="PD|Stash")
	bool HasItem(FName ItemID, int32 Quantity = 1) const;

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_StashItems();

	UFUNCTION()
	void OnRep_StashConfig();
};
