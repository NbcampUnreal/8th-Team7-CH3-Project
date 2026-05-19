#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Widgets/Inventory/PDInventoryDragDropOperation.h"
#include "PDSecureContainerWidget.generated.h"

class UUniformGridPanel;
class UPDInventoryComponent;
class UPDInventorySlotWidget;
class UPDSecureContainerComponent;

UCLASS(BlueprintType, Blueprintable)
class PROJECTD_API UPDSecureContainerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "PD|SecureContainer")
	void RefreshSecureContainer();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|SecureContainer")
	TSubclassOf<UPDInventorySlotWidget> InventorySlotWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|SecureContainer")
	FName SecureGridWidgetName = TEXT("UniformGridPanel_SecureContainer");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|SecureContainer", meta = (ClampMin = "1"))
	int32 FallbackSlotCount = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|SecureContainer", meta = (ClampMin = "1"))
	int32 GridColumns = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|SecureContainer", meta = (ClampMin = "1.0"))
	float SlotWidth = 70.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|SecureContainer", meta = (ClampMin = "1.0"))
	float SlotHeight = 70.f;

	UFUNCTION()
	void HandleSecureSlotItemDropped(UPDInventorySlotWidget* SlotWidget, int32 TargetSlotIndex, UPDInventoryDragDropOperation* DragOperation);

private:
	void ResolveSecureGridPanel();
	void BindSecureContainerChanged();
	void UnbindSecureContainerChanged();

	UPDInventoryComponent* FindInventoryComponent() const;
	UPDSecureContainerComponent* FindSecureContainerComponent() const;

	UPROPERTY(Transient)
	TObjectPtr<UUniformGridPanel> SecureGridPanel;

	UPROPERTY(Transient)
	TObjectPtr<UPDSecureContainerComponent> BoundSecureContainerComponent;
};
