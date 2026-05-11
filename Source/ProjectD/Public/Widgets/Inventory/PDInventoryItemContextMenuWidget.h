#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Type/Types.h"
#include "PDInventoryItemContextMenuWidget.generated.h"

class UButton;
class UPDInventoryItemContextMenuWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPDOnInventoryContextMenuAction, UPDInventoryItemContextMenuWidget*, MenuWidget, int32, SlotIndex);

UCLASS(BlueprintType, Blueprintable)
class PROJECTD_API UPDInventoryItemContextMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "PD|Inventory|ContextMenu")
	void InitializeContextMenu(int32 InSlotIndex, const FPDInventorySlot& InSlotData);

	UFUNCTION(BlueprintPure, Category = "PD|Inventory|ContextMenu")
	int32 GetSlotIndex() const { return SlotIndex; }

	UFUNCTION(BlueprintPure, Category = "PD|Inventory|ContextMenu")
	const FPDInventorySlot& GetSlotData() const { return SlotData; }

	UPROPERTY(BlueprintAssignable, Category = "PD|Inventory|ContextMenu|Event")
	FPDOnInventoryContextMenuAction OnUseClicked;

	UPROPERTY(BlueprintAssignable, Category = "PD|Inventory|ContextMenu|Event")
	FPDOnInventoryContextMenuAction OnDropClicked;

	UPROPERTY(BlueprintAssignable, Category = "PD|Inventory|ContextMenu|Event")
	FPDOnInventoryContextMenuAction OnEquipClicked;

protected:
	virtual void NativeOnInitialized() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Inventory|ContextMenu")
	void OnContextMenuInitialized();

	UPROPERTY(BlueprintReadOnly, Category = "PD|Inventory|ContextMenu")
	int32 SlotIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "PD|Inventory|ContextMenu")
	FPDInventorySlot SlotData;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Inventory|ContextMenu|Widget", meta = (AllowPrivateAccess = "true"))
	FName UseButtonWidgetName = TEXT("Button_Use");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Inventory|ContextMenu|Widget", meta = (AllowPrivateAccess = "true"))
	FName DropButtonWidgetName = TEXT("Button_Drop");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Inventory|ContextMenu|Widget", meta = (AllowPrivateAccess = "true"))
	FName EquipButtonWidgetName = TEXT("Button_Equip");

private:
	void ResolveButtons();
	void RefreshButtonVisibility();

	UFUNCTION()
	void HandleUseButtonClicked();

	UFUNCTION()
	void HandleDropButtonClicked();

	UFUNCTION()
	void HandleEquipButtonClicked();

	UPROPERTY(Transient)
	TObjectPtr<UButton> UseButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> DropButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> EquipButton;
};
