#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Type/Types.h"
#include "PDSecureContainerComponent.generated.h"

class UPDInventoryComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPDOnSecureContainerChanged);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROJECTD_API UPDSecureContainerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPDSecureContainerComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|SecureContainer", meta = (ClampMin = "1"))
	int32 SlotCount = 1;

	UPROPERTY(BlueprintAssignable, Category = "PD|SecureContainer")
	FPDOnSecureContainerChanged OnSecureContainerChanged;

	UFUNCTION(BlueprintPure, Category = "PD|SecureContainer")
	int32 GetSlotCount() const { return FMath::Max(1, SlotCount); }

	UFUNCTION(BlueprintPure, Category = "PD|SecureContainer")
	float GetCurrentWeight() const;

	const TArray<FPDInventorySlot>& GetSecureItems() const { return SecureItems; }

	const FPDInventorySlot* GetSecureSlot(int32 SlotIndex) const;

	UFUNCTION(BlueprintCallable, Category = "PD|SecureContainer")
	void InitializeSecureContainer();

	UFUNCTION(BlueprintCallable, Category = "PD|SecureContainer")
	void SaveSecureContainer();

	UFUNCTION(BlueprintCallable, Category = "PD|SecureContainer")
	bool MoveSlotQuantityToSlot(int32 SourceSlotIndex, int32 TargetSlotIndex, int32 Quantity);

	UFUNCTION(BlueprintCallable, Category = "PD|SecureContainer")
	bool StoreInventorySlotQuantityToSlot(UPDInventoryComponent* SourceInventory, int32 SourceSlotIndex, int32 TargetSecureSlotIndex, int32 Quantity);

	UFUNCTION(BlueprintCallable, Category = "PD|SecureContainer")
	bool TakeSecureSlotQuantityToInventorySlot(UPDInventoryComponent* TargetInventory, int32 SecureSlotIndex, int32 TargetInventorySlotIndex, int32 Quantity);

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere, Category = "PD|SecureContainer")
	TArray<FPDInventorySlot> SecureItems;

	void RestoreSecureContainer();
	void ResizeSecureItems();
	void NotifySecureContainerChanged(bool bSaveToPlayerData = true);
};
