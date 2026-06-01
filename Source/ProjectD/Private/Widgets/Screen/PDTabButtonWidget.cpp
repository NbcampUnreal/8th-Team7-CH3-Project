#include "Widgets/Screen/PDTabButtonWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"

void UPDTabButtonWidget::InitializeTabButton(FGameplayTag InTabId, const FText& InLabel, UTexture2D* OptionalIcon, bool bInEnabled)
{
	TabId = InTabId;
	bEnabled = bInEnabled;

	if (TXT_Label)
	{
		TXT_Label->SetText(InLabel);
	}

	if (IMG_Icon)
	{
		if (OptionalIcon)
		{
			IMG_Icon->SetBrushFromTexture(OptionalIcon);
			IMG_Icon->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			IMG_Icon->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (BTN_Tab)
	{
		BTN_Tab->SetIsEnabled(bInEnabled);
	}

	BP_OnEnabledStateChanged(bInEnabled);
}

void UPDTabButtonWidget::SetSelected(bool bInSelected)
{
	if (bSelected == bInSelected)
	{
		return;
	}
	bSelected = bInSelected;
	BP_OnSelectionChanged(bSelected);
	OnSelectionChanged.Broadcast(TabId, bSelected);
}

void UPDTabButtonWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (BTN_Tab && !BTN_Tab->OnClicked.IsAlreadyBound(this, &UPDTabButtonWidget::HandleButtonClicked))
	{
		BTN_Tab->OnClicked.AddDynamic(this, &UPDTabButtonWidget::HandleButtonClicked);
	}
}

void UPDTabButtonWidget::NativeDestruct()
{
	if (BTN_Tab && BTN_Tab->OnClicked.IsAlreadyBound(this, &UPDTabButtonWidget::HandleButtonClicked))
	{
		BTN_Tab->OnClicked.RemoveDynamic(this, &UPDTabButtonWidget::HandleButtonClicked);
	}
	Super::NativeDestruct();
}

void UPDTabButtonWidget::HandleButtonClicked()
{
	if (!bEnabled)
	{
		return;
	}
	OnTabButtonClicked.Broadcast(TabId);
}