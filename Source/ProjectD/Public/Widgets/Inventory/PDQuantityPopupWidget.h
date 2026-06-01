#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Type/Types.h"
#include "PDQuantityPopupWidget.generated.h"

class USoundBase;
class UTextBlock;
class UButton;
class UWidget;
class UPDInventorySlotWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPDOnQuantityPopupConfirmed, int32, Quantity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPDOnQuantityPopupCancelled);

UCLASS(BlueprintType, Blueprintable)
class PROJECTD_API UPDQuantityPopupWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "PD|Quantity")
	void InitializeQuantityPopup(int32 InMaxQuantity, const FText& InTitle);

	UFUNCTION(BlueprintCallable, Category = "PD|Quantity")
	void InitializeQuantityPopupWithSlot(int32 InMaxQuantity, const FText& InTitle, const FPDInventorySlot& InPreviewSlot);

	UPROPERTY(BlueprintAssignable, Category = "PD|Quantity")
	FPDOnQuantityPopupConfirmed OnConfirmed;

	UPROPERTY(BlueprintAssignable, Category = "PD|Quantity")
	FPDOnQuantityPopupCancelled OnCancelled;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|UI Sound")
	TObjectPtr<USoundBase> ButtonClickSound;

	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Quantity|Widget")
	FName TextTitleWidgetName = TEXT("Text_Title");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Quantity|Widget")
	FName TextQuantityWidgetName = TEXT("Text_Quantity");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Quantity|Widget")
	FName TextMaxQuantityWidgetName = TEXT("Text_MaxQuantity");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Quantity|Widget")
	FName ButtonMinusWidgetName = TEXT("Button_Minus");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Quantity|Widget")
	FName ButtonPlusWidgetName = TEXT("Button_Plus");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Quantity|Widget")
	FName ButtonConfirmWidgetName = TEXT("Button_Buy");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Quantity|Widget")
	FName ButtonCancelWidgetName = TEXT("Button_Cancel");

private:
	UFUNCTION()
	void HandleMinusClicked();

	UFUNCTION()
	void HandlePlusClicked();

	UFUNCTION()
	void HandleConfirmClicked();

	UFUNCTION()
	void HandleCancelClicked();

	void ResolveWidgets();
	void ApplyPopupStateToWidgets();
	void RefreshQuantityWidgets();
	void SetCurrentQuantity(int32 InQuantity);
	UWidget* FindWidgetByExactNameOrPartialName(FName ExactName, const FString& PartialName) const;
	UPDInventorySlotWidget* FindPreviewSlotWidget() const;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TextTitleWidget;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TextQuantityWidget;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TextMaxQuantityWidget;

	UPROPERTY(Transient)
	TObjectPtr<UButton> ButtonMinusWidget;

	UPROPERTY(Transient)
	TObjectPtr<UButton> ButtonPlusWidget;

	UPROPERTY(Transient)
	TObjectPtr<UButton> ButtonConfirmWidget;

	UPROPERTY(Transient)
	TObjectPtr<UButton> ButtonCancelWidget;

	UPROPERTY(Transient)
	TObjectPtr<UPDInventorySlotWidget> PreviewSlotWidget;

	FText PopupTitle;
	FPDInventorySlot PreviewSlotData;
	bool bHasPendingPopupState = false;
	bool bHasPreviewSlotData = false;
	int32 MaxQuantity = 1;
	int32 CurrentQuantity = 1;
};
