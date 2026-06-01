#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "Items/Containers/PDInventoryComponent.h"
#include "PDMarketComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPDOnMarketChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPDOnTraderReputationChanged, int32, NewLevel, int32, NewExp);

/** Source legacy: row used when populating goods from a single data table. */
USTRUCT(BlueprintType)
struct FPDMarketGoodsRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|Market")
	FName ItemRowName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|Market")
	int32 Stock = -1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|Market")
	int32 OverridePrice = -1;
};

USTRUCT(BlueprintType)
struct FPDMarketEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|Market")
	TObjectPtr<UDataTable> ItemDataTable = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|Market")
	FName ItemRowName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|Market")
	int32 Stock = -1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|Market")
	int32 OverridePrice = -1;
};

USTRUCT(BlueprintType)
struct FPDMarketLevelData : public FTableRowBase
{
	GENERATED_BODY()


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|Market")
	int32 Level = 1;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|Market")
	int32 RequiredExp = 0;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|Market")
	EPDItemGrade MaxPurchasableGrade = EPDItemGrade::Grade1;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTD_API UPDMarketComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPDMarketComponent();

	virtual void BeginPlay() override;

	UPROPERTY(ReplicatedUsing=OnRep_MarketData, EditAnywhere, BlueprintReadWrite, Category="PD|Market")
	TArray<FPDMarketEntry> Goods;

	/** Source legacy: optional data table whose rows are FPDMarketGoodsRow used to populate Goods at BeginPlay. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="PD|Market")
	TObjectPtr<UDataTable> MarketGoodsDataTable = nullptr;

	/** Source legacy: data table used by FindItemData / ReloadGoodsFromDataTable to resolve PDItemData rows by name. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="PD|Market")
	TObjectPtr<UDataTable> ItemDataTable = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|Market|Level")
	TObjectPtr<UDataTable> MarketLevelDataTable = nullptr;

	UPROPERTY(ReplicatedUsing=OnRep_MarketData, EditAnywhere, BlueprintReadWrite, Category="PD|Market|Level", meta=(ClampMin="0"))
	int32 TraderReputationExp = 0;

	UPROPERTY(ReplicatedUsing=OnRep_MarketData, EditAnywhere, BlueprintReadWrite, Category="PD|Market|Level", meta=(ClampMin="1"))
	int32 TraderReputationLevel = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|Market|Level", meta=(ClampMin="0"))
	int32 BuyReputationExpPercent = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|Market|Level", meta=(ClampMin="0"))
	int32 SellReputationExpPercent = 2;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|Market|Price", meta=(ClampMin="0.0", ClampMax="1.0"))
	float SellPriceRate = 0.35f;

	UPROPERTY(BlueprintAssignable, Category="PD|Market")
	FPDOnMarketChanged OnMarketChanged;

	UPROPERTY(BlueprintAssignable, Category="PD|Market|Level")
	FPDOnTraderReputationChanged OnTraderReputationChanged;

	UFUNCTION(BlueprintCallable, Category="PD|Market")
	bool BuyEntry(UPDInventoryComponent* BuyerInventory, int32 EntryIndex, int32 Quantity = 1);

	UFUNCTION(BlueprintCallable, Category="PD|Market")
	bool SellInventorySlot(UPDInventoryComponent* SellerInventory, int32 SlotIndex, int32 Quantity = 1);

	UFUNCTION(BlueprintPure, Category="PD|Market")
	bool ResolveEntryItemData(const FPDMarketEntry& Entry, FPDItemData& OutItemData) const;

	/** Source legacy: search the local ItemDataTable for an item by row name. */
	const FPDItemData* FindItemData(FName ItemRowName) const;

	/** Source legacy: reload Goods from MarketGoodsDataTable when the component is configured with one. */
	UFUNCTION(BlueprintCallable, Category="PD|Market")
	void ReloadGoodsFromDataTable();

	UFUNCTION(BlueprintPure, Category="PD|Market")
	int32 GetEntryUnitPrice(const FPDMarketEntry& Entry) const;

	UFUNCTION(BlueprintPure, Category="PD|Market")
	int32 GetItemBuyPrice(const FPDItemData& ItemData) const;

	UFUNCTION(BlueprintPure, Category="PD|Market")
	int32 GetItemSellPrice(const FPDItemData& ItemData) const;

	UFUNCTION(BlueprintPure, Category="PD|Market|Level")
	bool CanBuyEntry(int32 EntryIndex) const;

	UFUNCTION(BlueprintPure, Category="PD|Market|Level")
	bool CanBuyEntryForInventory(int32 EntryIndex, const UPDInventoryComponent* BuyerInventory) const;

	UFUNCTION(BlueprintPure, Category="PD|Market|Level")
	bool CanBuyItemData(const FPDItemData& ItemData) const;

	UFUNCTION(BlueprintPure, Category="PD|Market|Level")
	bool CanBuyItemDataForInventory(const FPDItemData& ItemData, const UPDInventoryComponent* BuyerInventory) const;


	UFUNCTION(BlueprintPure, Category="PD|Market|Level")
	bool ShouldShowEntry(int32 EntryIndex) const;

	UFUNCTION(BlueprintPure, Category="PD|Market|Level")
	bool ShouldShowEntryForInventory(int32 EntryIndex, const UPDInventoryComponent* BuyerInventory) const;

	UFUNCTION(BlueprintPure, Category="PD|Market|Level")
	int32 GetRequiredTraderLevelForGrade(EPDItemGrade ItemGrade) const;

	UFUNCTION(BlueprintPure, Category="PD|Market|Level")
	int32 GetRequiredTraderLevelForEntry(int32 EntryIndex) const;

	UFUNCTION(BlueprintPure, Category="PD|Market|Level")
	EPDItemGrade GetMaxPurchasableGradeForLevel(int32 Level) const;

	UFUNCTION(BlueprintPure, Category="PD|Market|Level")
	int32 GetCurrentTraderLevelRequiredExp() const;

	UFUNCTION(BlueprintPure, Category="PD|Market|Level")
	int32 GetNextTraderLevelRequiredExp() const;

	UFUNCTION(BlueprintPure, Category="PD|Market|Level")
	int32 GetCurrentTraderLevelDisplayExp() const;

	UFUNCTION(BlueprintPure, Category="PD|Market|Level")
	int32 GetCurrentTraderLevelDisplayExpForInventory(const UPDInventoryComponent* BuyerInventory) const;

	UFUNCTION(BlueprintPure, Category="PD|Market|Level")
	int32 GetNextTraderLevelDisplayRequiredExp() const;

	UFUNCTION(BlueprintPure, Category="PD|Market|Level")
	int32 GetNextTraderLevelDisplayRequiredExpForInventory(const UPDInventoryComponent* BuyerInventory) const;

	UFUNCTION(BlueprintPure, Category="PD|Market|Level")
	int32 GetTraderReputationExpForInventory(const UPDInventoryComponent* BuyerInventory) const;

	UFUNCTION(BlueprintPure, Category="PD|Market|Level")
	int32 GetTraderReputationLevelForInventory(const UPDInventoryComponent* BuyerInventory) const;

	UFUNCTION(BlueprintCallable, Category="PD|Market|Level")
	void AddTraderReputationExp(int32 Amount);

	UFUNCTION(BlueprintCallable, Category="PD|Market|Level")
	void SyncTraderReputationFromSave();

	UFUNCTION(BlueprintCallable, Category="PD|Market|Level")
	void RecalculateTraderReputationLevel();

private:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_MarketData();

	FPDMarketEntry* FindEntryByItemID(FName ItemID);
	const UDataTable* GetResolvedMarketLevelDataTable() const;
	const FPDMarketLevelData* FindMarketLevelDataByLevel(int32 Level) const;
	class APDPlayerState* GetPlayerStateFromInventory(const UPDInventoryComponent* InventoryComponent) const;
	int32 CalculateTraderReputationLevelFromExp(int32 ReputationExp) const;
	void AddTraderReputationExpForInventory(UPDInventoryComponent* InventoryComponent, int32 Amount);
	void LoadTraderReputationFromSave();
	void SaveTraderReputationToSave() const;
	int32 CalculateReputationReward(int32 TotalPrice, int32 Percent) const;
};
