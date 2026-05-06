#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/DataTable.h"
#include "Interfaces/PDInteractable.h"
#include "PDItemBase.generated.h"

USTRUCT(BlueprintType)
struct FPDItemData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ItemID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* Icon = nullptr;
};

UCLASS(Abstract, Blueprintable)
class PROJECTD_API APDItemBase : public AActor, public IPDInteractable
{
	GENERATED_BODY()

public:
	APDItemBase();

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Item")
	UDataTable* ItemDataTable = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Item")
	FName ItemRowName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Item")
	FPDItemData CachedItemData;

public:
	virtual void Interact_Implementation(AActor* Interactor) override {}

	FORCEINLINE const FPDItemData& GetItemData() const { return CachedItemData; }

protected:
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Item")
	void OnItemDataLoaded();

private:
	void LoadItemData();
};
