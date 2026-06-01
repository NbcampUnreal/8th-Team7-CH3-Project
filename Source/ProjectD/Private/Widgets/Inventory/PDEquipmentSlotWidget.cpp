#include "Widgets/Inventory/PDEquipmentSlotWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Input/Events.h"
#include "Widgets/Inventory/PDInventoryDragDropOperation.h"

void UPDEquipmentSlotWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	ResolveWidgets();
	RefreshVisuals();
}

FReply UPDEquipmentSlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		OnEquipmentSlotRightClicked.Broadcast(this, SlotType);
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}


bool UPDEquipmentSlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	if (UPDInventoryDragDropOperation* DragOperation = Cast<UPDInventoryDragDropOperation>(InOperation))
	{
		if (DragOperation->IsValidPayload() && OnEquipmentSlotItemDropped.IsBound())
		{
			OnEquipmentSlotItemDropped.Broadcast(this, SlotType, DragOperation);
			return true;
		}
	}

	return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
}

void UPDEquipmentSlotWidget::InitializeEquipmentSlot(EPDEquipmentSlotType InSlotType)
{
	SlotType = InSlotType;
	ResolveWidgets();
	RefreshVisuals();
}

void UPDEquipmentSlotWidget::SetEquippedItem(const FPDInventorySlot& InItemSlot)
{
	EquippedItem = InItemSlot;
	ResolveWidgets();
	RefreshVisuals();
}

void UPDEquipmentSlotWidget::ClearEquippedItem()
{
	EquippedItem.Clear();
	ResolveWidgets();
	RefreshVisuals();
}

void UPDEquipmentSlotWidget::ResolveWidgets()
{
	if (!WidgetTree)
	{
		return;
	}

	if (!TextItemNameWidget && !TextItemNameWidgetName.IsNone())
	{
		TextItemNameWidget = Cast<UTextBlock>(WidgetTree->FindWidget(TextItemNameWidgetName));
	}

	if (!TextSlotNameWidget && !TextSlotNameWidgetName.IsNone())
	{
		TextSlotNameWidget = Cast<UTextBlock>(WidgetTree->FindWidget(TextSlotNameWidgetName));
		if (!TextSlotNameWidget)
		{
			// Backward compatibility for older WBP_EquipmentSlot layouts.
			TextSlotNameWidget = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("Text_SlotName")));
		}
	}

	if (!ImageItemIconWidget && !ImageItemIconWidgetName.IsNone())
	{
		ImageItemIconWidget = Cast<UImage>(WidgetTree->FindWidget(ImageItemIconWidgetName));
	}
}

void UPDEquipmentSlotWidget::RefreshVisuals()
{
	ResolveWidgets();

	const bool bHasEquippedItem = !EquippedItem.IsEmpty();

	if (TextSlotNameWidget)
	{
		TextSlotNameWidget->SetText(GetDefaultSlotLabel());
		TextSlotNameWidget->SetVisibility(bHasEquippedItem ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}

	if (!bHasEquippedItem)
	{
		if (TextItemNameWidget)
		{
			TextItemNameWidget->SetText(FText::GetEmpty());
			TextItemNameWidget->SetVisibility(ESlateVisibility::Collapsed);
		}

		if (ImageItemIconWidget)
		{
			ImageItemIconWidget->SetBrushFromTexture(nullptr);
			ImageItemIconWidget->SetVisibility(ESlateVisibility::Hidden);
		}

		return;
	}

	const FText DisplayName = EquippedItem.ItemData.DisplayName.IsEmpty()
		? FText::FromName(EquippedItem.ItemData.ItemID)
		: EquippedItem.ItemData.DisplayName;

	if (TextItemNameWidget)
	{
		TextItemNameWidget->SetText(DisplayName);
		TextItemNameWidget->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (ImageItemIconWidget)
	{
		ImageItemIconWidget->SetBrushFromTexture(EquippedItem.ItemData.Icon);
		ImageItemIconWidget->SetVisibility(EquippedItem.ItemData.Icon ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}
}

FText UPDEquipmentSlotWidget::GetDefaultSlotLabel() const
{
	switch (SlotType)
	{
	case EPDEquipmentSlotType::Weapon:
		return FText::FromString(TEXT("Weapon"));
	case EPDEquipmentSlotType::Head:
		return FText::FromString(TEXT("Head"));
	case EPDEquipmentSlotType::Armor:
		return FText::FromString(TEXT("Armor"));
	case EPDEquipmentSlotType::Bag:
		return FText::FromString(TEXT("Bag"));
	default:
		return FText::FromString(TEXT("Equipment"));
	}
}
