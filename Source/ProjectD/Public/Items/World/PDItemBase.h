#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/PDInteractable.h"
#include "Type/Types.h"
#include "PDItemBase.generated.h"

class UDataTable;
class USphereComponent;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class PROJECTD_API APDItemBase : public AActor, public IPDInteractable
{
	GENERATED_BODY()

public:
	APDItemBase();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Item")
	TObjectPtr<USphereComponent> PickupCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Item")
	TObjectPtr<UStaticMeshComponent> ItemMesh;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Item")
	UDataTable* ItemDataTable = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Item")
	FName ItemRowName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Item")
	FPDItemData CachedItemData;

public:
	virtual void Interact_Implementation(AActor* Interactor) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Item", meta = (ClampMin = "1"))
	int32 Quantity = 1;

	FORCEINLINE const FPDItemData& GetItemData() const { return CachedItemData; }

protected:
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Item")
	void OnItemDataLoaded();

	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Item")
	void OnPickupSucceeded(int32 AddedQuantity, int32 RemainingQuantity);

	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Item")
	void OnPickupFailed();

private:
	void LoadItemData();
};
