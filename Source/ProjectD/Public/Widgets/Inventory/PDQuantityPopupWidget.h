#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Input/Events.h"
#include "PDQuantityPopupWidget.generated.h"

class UTextBlock;
class UEditableTextBox;
class UEditableText;
class UButton;
class UWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPDOnQuantityPopupConfirmed, int32, Quantity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPDOnQuantityPopupCancelled);

UCLASS(BlueprintType, Blueprintable)
class PROJECTD_API UPDQuantityPopupWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "PD|Quantity")
	void InitializeQuantityPopup(int32 InMaxQuantity, const FText& InTitle);

	UPROPERTY(BlueprintAssignable, Category = "PD|Quantity")
	FPDOnQuantityPopupConfirmed OnConfirmed;

	UPROPERTY(BlueprintAssignable, Category = "PD|Quantity")
	FPDOnQuantityPopupCancelled OnCancelled;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Quantity|Widget")
	FName TextTitleWidgetName = TEXT("Text_Title");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Quantity|Widget")
	FName TextMaxQuantityWidgetName = TEXT("Text_MaxQuantity");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Quantity|Widget")
	FName EditableQuantityWidgetName = TEXT("EditableTextBox_Quantity");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Quantity|Widget")
	FName ButtonConfirmWidgetName = TEXT("Button_Confirm");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Quantity|Widget")
	FName ButtonCancelWidgetName = TEXT("Button_Cancel");

private:
	UFUNCTION()
	void HandleConfirmClicked();

	UFUNCTION()
	void HandleQuantityTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);

	UFUNCTION()
	void HandleCancelClicked();

	void ResolveWidgets();
	void ApplyPopupStateToWidgets();
	UWidget* FindWidgetByExactNameOrPartialName(FName ExactName, const FString& PartialName) const;
	FText GetQuantityInputText() const;
	void SetQuantityInputText(const FText& InText);
	void FocusQuantityInput();
	int32 GetInputQuantity() const;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TextTitleWidget;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TextMaxQuantityWidget;

	UPROPERTY(Transient)
	TObjectPtr<UEditableTextBox> EditableQuantityTextBoxWidget;

	UPROPERTY(Transient)
	TObjectPtr<UEditableText> EditableQuantityTextWidget;

	UPROPERTY(Transient)
	TObjectPtr<UButton> ButtonConfirmWidget;

	UPROPERTY(Transient)
	TObjectPtr<UButton> ButtonCancelWidget;

	FText PopupTitle;
	bool bHasPendingPopupState = false;
	int32 MaxQuantity = 1;
};
