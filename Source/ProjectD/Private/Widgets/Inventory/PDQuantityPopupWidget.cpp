#include "Widgets/Inventory/PDQuantityPopupWidget.h"
#include "Widgets/PDWidgetSoundLibrary.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Widgets/Inventory/PDInventorySlotWidget.h"

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
	CurrentQuantity = 1;
	PopupTitle = InTitle;
	bHasPreviewSlotData = false;
	bHasPendingPopupState = true;

	ResolveWidgets();
	ApplyPopupStateToWidgets();
}

void UPDQuantityPopupWidget::InitializeQuantityPopupWithSlot(int32 InMaxQuantity, const FText& InTitle, const FPDInventorySlot& InPreviewSlot)
{
	MaxQuantity = FMath::Max(1, InMaxQuantity);
	CurrentQuantity = 1;
	PopupTitle = InTitle;
	PreviewSlotData = InPreviewSlot;
	bHasPreviewSlotData = !InPreviewSlot.IsEmpty();
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

	if (!TextQuantityWidget)
	{
		TextQuantityWidget = Cast<UTextBlock>(FindWidgetByExactNameOrPartialName(TextQuantityWidgetName, TEXT("Quantity")));
	}

	if (!TextMaxQuantityWidget)
	{
		TextMaxQuantityWidget = Cast<UTextBlock>(FindWidgetByExactNameOrPartialName(TextMaxQuantityWidgetName, TEXT("Max")));
	}

	if (!ButtonMinusWidget)
	{
		ButtonMinusWidget = Cast<UButton>(FindWidgetByExactNameOrPartialName(ButtonMinusWidgetName, TEXT("Minus")));
	}

	if (ButtonMinusWidget)
	{
		ButtonMinusWidget->OnClicked.RemoveDynamic(this, &UPDQuantityPopupWidget::HandleMinusClicked);
		ButtonMinusWidget->OnClicked.AddUniqueDynamic(this, &UPDQuantityPopupWidget::HandleMinusClicked);
	}

	if (!ButtonPlusWidget)
	{
		ButtonPlusWidget = Cast<UButton>(FindWidgetByExactNameOrPartialName(ButtonPlusWidgetName, TEXT("Plus")));
	}

	if (ButtonPlusWidget)
	{
		ButtonPlusWidget->OnClicked.RemoveDynamic(this, &UPDQuantityPopupWidget::HandlePlusClicked);
		ButtonPlusWidget->OnClicked.AddUniqueDynamic(this, &UPDQuantityPopupWidget::HandlePlusClicked);
	}

	if (!ButtonConfirmWidget)
	{
		ButtonConfirmWidget = Cast<UButton>(FindWidgetByExactNameOrPartialName(ButtonConfirmWidgetName, TEXT("Buy")));
	}

	if (!ButtonConfirmWidget)
	{
		ButtonConfirmWidget = Cast<UButton>(FindWidgetByExactNameOrPartialName(NAME_None, TEXT("Confirm")));
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

	if (!PreviewSlotWidget)
	{
		PreviewSlotWidget = FindPreviewSlotWidget();
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
		TextMaxQuantityWidget->SetText(FText::Format(NSLOCTEXT("PDInventory", "QuantityPopupMax", "Max {0}"), FText::AsNumber(MaxQuantity)));
	}

	if (PreviewSlotWidget && bHasPreviewSlotData)
	{
		FPDInventorySlot DisplaySlot = PreviewSlotData;
		DisplaySlot.Quantity = MaxQuantity;
		PreviewSlotWidget->SetSlotData(DisplaySlot, INDEX_NONE);
	}

	SetCurrentQuantity(FMath::Clamp(CurrentQuantity, 1, MaxQuantity));
}

void UPDQuantityPopupWidget::RefreshQuantityWidgets()
{
	if (TextQuantityWidget)
	{
		TextQuantityWidget->SetText(FText::AsNumber(CurrentQuantity));
	}

	if (ButtonMinusWidget)
	{
		ButtonMinusWidget->SetIsEnabled(CurrentQuantity > 1);
	}

	if (ButtonPlusWidget)
	{
		ButtonPlusWidget->SetIsEnabled(CurrentQuantity < MaxQuantity);
	}

	if (ButtonConfirmWidget)
	{
		ButtonConfirmWidget->SetIsEnabled(CurrentQuantity >= 1 && CurrentQuantity <= MaxQuantity);
	}
}

void UPDQuantityPopupWidget::SetCurrentQuantity(int32 InQuantity)
{
	CurrentQuantity = FMath::Clamp(InQuantity, 1, MaxQuantity);
	RefreshQuantityWidgets();
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
			if (!FoundWidget->GetTypedOuter<UPDInventorySlotWidget>())
			{
				return FoundWidget;
			}
		}
	}

	UWidget* FoundByPartialName = nullptr;
	if (!PartialName.IsEmpty())
	{
		WidgetTree->ForEachWidget([&FoundByPartialName, &PartialName](UWidget* Widget)
		{
				if (!FoundByPartialName && Widget && !Widget->GetTypedOuter<UPDInventorySlotWidget>() && Widget->GetName().Contains(PartialName))
			{
				FoundByPartialName = Widget;
			}
		});
	}

	return FoundByPartialName;
}

UPDInventorySlotWidget* UPDQuantityPopupWidget::FindPreviewSlotWidget() const
{
	if (!WidgetTree)
	{
		return nullptr;
	}

	UPDInventorySlotWidget* FoundSlotWidget = nullptr;
	WidgetTree->ForEachWidget([&FoundSlotWidget](UWidget* Widget)
	{
		if (!FoundSlotWidget)
		{
			FoundSlotWidget = Cast<UPDInventorySlotWidget>(Widget);
		}
	});

	return FoundSlotWidget;
}

void UPDQuantityPopupWidget::HandleMinusClicked()
{
	UPDWidgetSoundLibrary::PlayUISound2D(this, ButtonClickSound);

	SetCurrentQuantity(CurrentQuantity - 1);
}

void UPDQuantityPopupWidget::HandlePlusClicked()
{
	UPDWidgetSoundLibrary::PlayUISound2D(this, ButtonClickSound);

	SetCurrentQuantity(CurrentQuantity + 1);
}

void UPDQuantityPopupWidget::HandleConfirmClicked()
{
	UPDWidgetSoundLibrary::PlayUISound2D(this, ButtonClickSound);

	OnConfirmed.Broadcast(CurrentQuantity);
	RemoveFromParent();
}

void UPDQuantityPopupWidget::HandleCancelClicked()
{
	UPDWidgetSoundLibrary::PlayUISound2D(this, ButtonClickSound);

	OnCancelled.Broadcast();
	RemoveFromParent();
}
