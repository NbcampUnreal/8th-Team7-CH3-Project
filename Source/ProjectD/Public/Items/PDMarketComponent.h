#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "Items/PDInventoryComponent.h"
#include "PDMarketComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPDOnMarketChanged);

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

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTD_API UPDMarketComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPDMarketComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|Market")
	TArray<FPDMarketEntry> Goods;

	UPROPERTY(BlueprintAssignable, Category="PD|Market")
	FPDOnMarketChanged OnMarketChanged;

	UFUNCTION(BlueprintCallable, Category="PD|Market")
	bool BuyEntry(UPDInventoryComponent* BuyerInventory, int32 EntryIndex, int32 Quantity = 1);

	UFUNCTION(BlueprintCallable, Category="PD|Market")
	bool SellInventorySlot(UPDInventoryComponent* SellerInventory, int32 SlotIndex, int32 Quantity = 1);

	UFUNCTION(BlueprintPure, Category="PD|Market")
	bool ResolveEntryItemData(const FPDMarketEntry& Entry, FPDItemData& OutItemData) const;

	UFUNCTION(BlueprintPure, Category="PD|Market")
	int32 GetEntryUnitPrice(const FPDMarketEntry& Entry) const;

private:
	FPDMarketEntry* FindEntryByItemID(FName ItemID);
};
