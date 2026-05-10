#include "Widgets/Inventory/PDQuantityPopupWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/EditableText.h"
#include "Components/TextBlock.h"
#include "Input/Events.h"

void UPDQuantityPopupWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	ResolveWidgets();
}

void UPDQuantityPopupWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ResolveWidgets();
}

void UPDQuantityPopupWidget::InitializeQuantityPopup(int32 InMaxQuantity, const FText& InTitle)
{
	MaxQuantity = FMath::Max(1, InMaxQuantity);
	PopupTitle = InTitle;
	bHasPendingPopupState = true;

	ResolveWidgets();
	ApplyPopupStateToWidgets();
}

void UPDQuantityPopupWidget::ResolveWidgets()
{
	if (!WidgetTree)
	{
		return;
	}

	if (!TextTitleWidget)
	{
		TextTitleWidget = Cast<UTextBlock>(FindWidgetByExactNameOrPartialName(TextTitleWidgetName, TEXT("Title")));
	}

	if (!TextMaxQuantityWidget)
	{
		TextMaxQuantityWidget = Cast<UTextBlock>(FindWidgetByExactNameOrPartialName(TextMaxQuantityWidgetName, TEXT("Max")));
	}

	if (!EditableQuantityTextBoxWidget && !EditableQuantityTextWidget)
	{
		if (UWidget* QuantityWidget = FindWidgetByExactNameOrPartialName(EditableQuantityWidgetName, TEXT("Quantity")))
		{
			EditableQuantityTextBoxWidget = Cast<UEditableTextBox>(QuantityWidget);
			EditableQuantityTextWidget = Cast<UEditableText>(QuantityWidget);
		}
	}

	if (!EditableQuantityTextBoxWidget && !EditableQuantityTextWidget)
	{
		WidgetTree->ForEachWidget([this](UWidget* Widget)
		{
			if (!EditableQuantityTextBoxWidget)
			{
				EditableQuantityTextBoxWidget = Cast<UEditableTextBox>(Widget);
			}

			if (!EditableQuantityTextWidget)
			{
				EditableQuantityTextWidget = Cast<UEditableText>(Widget);
			}
		});
	}

	if (EditableQuantityTextBoxWidget)
	{
		EditableQuantityTextBoxWidget->OnTextCommitted.RemoveDynamic(this, &UPDQuantityPopupWidget::HandleQuantityTextCommitted);
		EditableQuantityTextBoxWidget->OnTextCommitted.AddUniqueDynamic(this, &UPDQuantityPopupWidget::HandleQuantityTextCommitted);
	}

	if (EditableQuantityTextWidget)
	{
		EditableQuantityTextWidget->OnTextCommitted.RemoveDynamic(this, &UPDQuantityPopupWidget::HandleQuantityTextCommitted);
		EditableQuantityTextWidget->OnTextCommitted.AddUniqueDynamic(this, &UPDQuantityPopupWidget::HandleQuantityTextCommitted);
	}

	if (!ButtonConfirmWidget)
	{
		ButtonConfirmWidget = Cast<UButton>(FindWidgetByExactNameOrPartialName(ButtonConfirmWidgetName, TEXT("Confirm")));
	}

	if (!ButtonConfirmWidget)
	{
		ButtonConfirmWidget = Cast<UButton>(FindWidgetByExactNameOrPartialName(NAME_None, TEXT("OK")));
	}

	if (ButtonConfirmWidget)
	{
		ButtonConfirmWidget->OnClicked.RemoveDynamic(this, &UPDQuantityPopupWidget::HandleConfirmClicked);
		ButtonConfirmWidget->OnClicked.AddUniqueDynamic(this, &UPDQuantityPopupWidget::HandleConfirmClicked);
	}

	if (!ButtonCancelWidget)
	{
		ButtonCancelWidget = Cast<UButton>(FindWidgetByExactNameOrPartialName(ButtonCancelWidgetName, TEXT("Cancel")));
	}

	if (ButtonCancelWidget)
	{
		ButtonCancelWidget->OnClicked.RemoveDynamic(this, &UPDQuantityPopupWidget::HandleCancelClicked);
		ButtonCancelWidget->OnClicked.AddUniqueDynamic(this, &UPDQuantityPopupWidget::HandleCancelClicked);
	}

	ApplyPopupStateToWidgets();
}

void UPDQuantityPopupWidget::ApplyPopupStateToWidgets()
{
	if (!bHasPendingPopupState)
	{
		return;
	}

	if (TextTitleWidget)
	{
		TextTitleWidget->SetText(PopupTitle);
	}

	if (TextMaxQuantityWidget)
	{
		TextMaxQuantityWidget->SetText(FText::Format(NSLOCTEXT("PDInventory", "QuantityPopupMax", "Max: {0}"), FText::AsNumber(MaxQuantity)));
	}

	SetQuantityInputText(FText::AsNumber(1));
	FocusQuantityInput();
}

UWidget* UPDQuantityPopupWidget::FindWidgetByExactNameOrPartialName(FName ExactName, const FString& PartialName) const
{
	if (!WidgetTree)
	{
		return nullptr;
	}

	if (!ExactName.IsNone())
	{
		if (UWidget* FoundWidget = WidgetTree->FindWidget(ExactName))
		{
			return FoundWidget;
		}
	}

	UWidget* FoundByPartialName = nullptr;
	if (!PartialName.IsEmpty())
	{
		WidgetTree->ForEachWidget([&FoundByPartialName, &PartialName](UWidget* Widget)
		{
			if (!FoundByPartialName && Widget && Widget->GetName().Contains(PartialName))
			{
				FoundByPartialName = Widget;
			}
		});
	}

	return FoundByPartialName;
}

FText UPDQuantityPopupWidget::GetQuantityInputText() const
{
	if (EditableQuantityTextBoxWidget)
	{
		return EditableQuantityTextBoxWidget->GetText();
	}

	if (EditableQuantityTextWidget)
	{
		return EditableQuantityTextWidget->GetText();
	}

	return FText::GetEmpty();
}

void UPDQuantityPopupWidget::SetQuantityInputText(const FText& InText)
{
	if (EditableQuantityTextBoxWidget)
	{
		EditableQuantityTextBoxWidget->SetText(InText);
	}

	if (EditableQuantityTextWidget)
	{
		EditableQuantityTextWidget->SetText(InText);
	}
}

void UPDQuantityPopupWidget::FocusQuantityInput()
{
	if (EditableQuantityTextBoxWidget)
	{
		EditableQuantityTextBoxWidget->SetKeyboardFocus();
		return;
	}

	if (EditableQuantityTextWidget)
	{
		EditableQuantityTextWidget->SetKeyboardFocus();
	}
}

int32 UPDQuantityPopupWidget::GetInputQuantity() const
{
	const FText QuantityInputText = GetQuantityInputText();
	if (QuantityInputText.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("PDQuantityPopupWidget: Quantity input widget was not found. Expected widget name: %s"), *EditableQuantityWidgetName.ToString());
		return 1;
	}

	const FString QuantityString = QuantityInputText.ToString().TrimStartAndEnd();
	if (QuantityString.IsEmpty())
	{
		return 1;
	}

	const int32 ParsedQuantity = FCString::Atoi(*QuantityString);
	return FMath::Clamp(ParsedQuantity, 1, MaxQuantity);
}

void UPDQuantityPopupWidget::HandleConfirmClicked()
{
	OnConfirmed.Broadcast(GetInputQuantity());
	RemoveFromParent();
}

void UPDQuantityPopupWidget::HandleQuantityTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	if (CommitMethod == ETextCommit::OnEnter)
	{
		HandleConfirmClicked();
	}
}

void UPDQuantityPopupWidget::HandleCancelClicked()
{
	OnCancelled.Broadcast();
	RemoveFromParent();
}
