#include "Widgets/Inventory/PDInventorySlotWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Input/Events.h"

void UPDInventorySlotWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	ResolveTextWidgets();
	RefreshVisuals();
}

FReply UPDInventorySlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bLastClickWithControl = InMouseEvent.IsControlDown();
		OnSlotLeftClicked.Broadcast(this, SlotIndex);

		if (!bLastClickWithControl && !SlotData.IsEmpty())
		{
			OpenItemDropdown();
		}

		return FReply::Handled();
	}

	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		bLastClickWithControl = InMouseEvent.IsControlDown();
		OnSlotRightClicked.Broadcast(this, SlotIndex);

		if (!SlotData.IsEmpty())
		{
			OpenUseDropMenu();
		}

		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UPDInventorySlotWidget::SetSlotData(const FPDInventorySlot& InSlotData, int32 InSlotIndex)
{
	SlotData = InSlotData;
	SlotIndex = InSlotIndex;
	ResolveTextWidgets();
	RefreshVisuals();
}

void UPDInventorySlotWidget::ClearSlotData(int32 InSlotIndex)
{
	SlotData.Clear();
	SlotIndex = InSlotIndex;
	ResolveTextWidgets();
	RefreshVisuals();
}

void UPDInventorySlotWidget::ResolveTextWidgets()
{
	if (!WidgetTree)
	{
		return;
	}

	if (!TextItemNameWidget && !TextItemNameWidgetName.IsNone())
	{
		TextItemNameWidget = Cast<UTextBlock>(WidgetTree->FindWidget(TextItemNameWidgetName));
	}

	if (!TextQuantityWidget && !TextQuantityWidgetName.IsNone())
	{
		TextQuantityWidget = Cast<UTextBlock>(WidgetTree->FindWidget(TextQuantityWidgetName));
	}

	if (!ImageItemIconWidget && !ImageItemIconWidgetName.IsNone())
	{
		ImageItemIconWidget = Cast<UImage>(WidgetTree->FindWidget(ImageItemIconWidgetName));
	}
}

void UPDInventorySlotWidget::RefreshVisuals()
{
	ResolveTextWidgets();

	if (SlotData.IsEmpty())
	{
		ClearTooltip();

		if (TextItemNameWidget)
		{
			TextItemNameWidget->SetText(FText::GetEmpty());
			TextItemNameWidget->SetVisibility(ESlateVisibility::Collapsed);
		}

		if (TextQuantityWidget)
		{
			TextQuantityWidget->SetText(FText::GetEmpty());
		}

		if (ImageItemIconWidget)
		{
			ImageItemIconWidget->SetBrushFromTexture(nullptr);
			ImageItemIconWidget->SetVisibility(ESlateVisibility::Hidden);
		}

		return;
	}

	const FText DisplayName = SlotData.ItemData.DisplayName.IsEmpty() ? FText::FromName(SlotData.ItemData.ItemID) : SlotData.ItemData.DisplayName;
	const FText Description = SlotData.ItemData.Description;

	if (TextItemNameWidget)
	{
		TextItemNameWidget->SetText(DisplayName);
		TextItemNameWidget->SetVisibility(ESlateVisibility::Visible);
	}

	if (TextQuantityWidget)
	{
		TextQuantityWidget->SetText(SlotData.Quantity > 1 ? FText::AsNumber(SlotData.Quantity) : FText::GetEmpty());
	}

	if (ImageItemIconWidget)
	{
		ImageItemIconWidget->SetBrushFromTexture(SlotData.ItemData.Icon);
		ImageItemIconWidget->SetVisibility(SlotData.ItemData.Icon ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}

	ApplyTooltip(DisplayName, Description);
}

void UPDInventorySlotWidget::ApplyTooltip(const FText& DisplayName, const FText& Description)
{
	if (!TooltipWidgetClass)
	{
		SetToolTipText(Description.IsEmpty()
			? DisplayName
			: FText::Format(NSLOCTEXT("PDInventory", "ItemTooltipNameDescription", "{0}\n{1}"), DisplayName, Description));
		return;
	}

	UUserWidget* TooltipWidget = GetOwningPlayer()
		? CreateWidget<UUserWidget>(GetOwningPlayer(), TooltipWidgetClass)
		: CreateWidget<UUserWidget>(GetWorld(), TooltipWidgetClass);

	if (!TooltipWidget)
	{
		SetToolTipText(Description.IsEmpty()
			? DisplayName
			: FText::Format(NSLOCTEXT("PDInventory", "ItemTooltipNameDescription", "{0}\n{1}"), DisplayName, Description));
		return;
	}

	if (TooltipWidget->WidgetTree)
	{
		if (UTextBlock* TooltipNameText = Cast<UTextBlock>(TooltipWidget->WidgetTree->FindWidget(TooltipItemNameWidgetName)))
		{
			TooltipNameText->SetText(DisplayName);
		}

		if (UTextBlock* TooltipDescriptionText = Cast<UTextBlock>(TooltipWidget->WidgetTree->FindWidget(TooltipDescriptionWidgetName)))
		{
			TooltipDescriptionText->SetText(Description);
		}
	}

	SetToolTip(TooltipWidget);
}

void UPDInventorySlotWidget::ClearTooltip()
{
	SetToolTip(nullptr);
	SetToolTipText(FText::GetEmpty());
}
