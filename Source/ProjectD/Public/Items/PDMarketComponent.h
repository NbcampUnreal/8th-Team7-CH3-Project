#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "Items/PDInventoryComponent.h"
#include "PDMarketComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPDOnMarketChanged);

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="PD|Market")
	FName ItemRowName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="PD|Market")
	int32 Stock = -1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="PD|Market")
	int32 OverridePrice = -1;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTD_API UPDMarketComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPDMarketComponent();

	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="PD|Market|Data")
	TObjectPtr<UDataTable> MarketGoodsDataTable = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="PD|Market|Data")
	TObjectPtr<UDataTable> ItemDataTable = nullptr;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="PD|Market")
	TArray<FPDMarketEntry> Goods;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|Market|Price", meta=(ClampMin="0.0", ClampMax="1.0"))
	float SellPriceRate = 0.35f;

	UPROPERTY(BlueprintAssignable, Category="PD|Market")
	FPDOnMarketChanged OnMarketChanged;

	UFUNCTION(BlueprintCallable, Category="PD|Market")
	void ReloadGoodsFromDataTable();

	UFUNCTION(BlueprintCallable, Category="PD|Market")
	bool BuyEntry(UPDInventoryComponent* BuyerInventory, int32 EntryIndex, int32 Quantity = 1);

	UFUNCTION(BlueprintCallable, Category="PD|Market")
	bool SellInventorySlot(UPDInventoryComponent* SellerInventory, int32 SlotIndex, int32 Quantity = 1);

	UFUNCTION(BlueprintPure, Category="PD|Market")
	bool ResolveEntryItemData(const FPDMarketEntry& Entry, FPDItemData& OutItemData) const;

	UFUNCTION(BlueprintPure, Category="PD|Market")
	int32 GetEntryUnitPrice(const FPDMarketEntry& Entry) const;

	UFUNCTION(BlueprintPure, Category="PD|Market")
	int32 GetItemBuyPrice(const FPDItemData& ItemData) const;

	UFUNCTION(BlueprintPure, Category="PD|Market")
	int32 GetItemSellPrice(const FPDItemData& ItemData) const;

	UFUNCTION(BlueprintPure, Category="PD|Market")
	bool CanBuyEntry(int32 EntryIndex) const;

	UFUNCTION(BlueprintPure, Category="PD|Market")
	bool CanBuyItemData(const FPDItemData& ItemData) const;

	UFUNCTION(BlueprintPure, Category="PD|Market")
	bool ShouldShowEntry(int32 EntryIndex) const;

private:
	FPDMarketEntry* FindEntryByItemID(FName ItemID);
	const FPDItemData* FindItemData(FName ItemRowName) const;
};
