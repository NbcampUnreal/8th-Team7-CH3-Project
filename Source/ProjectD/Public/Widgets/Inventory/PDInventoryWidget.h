
#pragma once

#include "CoreMinimal.h"
#include "Widgets/PDActivatableBase.h"
#include "PDInventoryWidget.generated.h"

class UUniformGridPanel;
class UPDInventoryComponent;
class UPDStashComponent;
class UPDInventorySlotWidget;
class UUserWidget;
class UWidgetTree;
class UTextBlock;
class UPDQuantityPopupWidget;

UCLASS(BlueprintType, Blueprintable)
class PROJECTD_API UPDInventoryWidget : public UPDActivatableBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "PD|Inventory")
	void RefreshInventoryGrid();

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Inventory")
	TSubclassOf<UUserWidget> InventorySlotWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Inventory")
	int32 FallbackGridColumns = 5;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Inventory")
	int32 FallbackGridRows = 4;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Inventory")
	FName InventoryGridWidgetName = TEXT("UniformGridPanel_InventoryGrid");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Inventory")
	FName GoldTextWidgetName = TEXT("Text_Gold");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Inventory")
	TSubclassOf<UPDQuantityPopupWidget> QuantityPopupWidgetClass;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "PD|Inventory")
	TObjectPtr<UUniformGridPanel> InventoryGridPanel;
	
	UFUNCTION()
	void HandleInventorySlotLeftClicked(UPDInventorySlotWidget* SlotWidget, int32 ClickedSlotIndex);

	UFUNCTION()
	void HandleQuantityConfirmed(int32 Quantity);

private:
	void ResolveInventoryGridPanel();
	void RefreshGoldText();
	void ExecuteInventoryQuickAction(int32 SlotIndex, int32 Quantity);
	void OpenQuantityPopup(int32 SlotIndex, int32 MaxQuantity, const FText& Title);
	UPDInventoryComponent* FindInventoryComponent() const;
	UPDStashComponent* FindStashComponent() const;
	void BindInventoryChanged();
	void UnbindInventoryChanged();

	UPROPERTY(Transient)
	TObjectPtr<UPDInventoryComponent> BoundInventoryComponent;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> GoldTextWidget;

	UPROPERTY(Transient)
	TObjectPtr<UPDQuantityPopupWidget> ActiveQuantityPopup;

	int32 PendingSlotIndex = INDEX_NONE;
};
