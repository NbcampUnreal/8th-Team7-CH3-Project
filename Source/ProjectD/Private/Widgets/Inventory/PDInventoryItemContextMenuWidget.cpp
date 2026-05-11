#include "Widgets/Inventory/PDInventoryItemContextMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"

void UPDInventoryItemContextMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	ResolveButtons();
	RefreshButtonVisibility();
}

void UPDInventoryItemContextMenuWidget::InitializeContextMenu(int32 InSlotIndex, const FPDInventorySlot& InSlotData)
{
	SlotIndex = InSlotIndex;
	SlotData = InSlotData;

	ResolveButtons();
	RefreshButtonVisibility();
	OnContextMenuInitialized();
}

void UPDInventoryItemContextMenuWidget::ResolveButtons()
{
	if (!WidgetTree)
	{
		return;
	}

	if (!UseButton && !UseButtonWidgetName.IsNone())
	{
		UseButton = Cast<UButton>(WidgetTree->FindWidget(UseButtonWidgetName));
		if (UseButton)
		{
			UseButton->OnClicked.AddUniqueDynamic(this, &UPDInventoryItemContextMenuWidget::HandleUseButtonClicked);
		}
	}

	if (!DropButton && !DropButtonWidgetName.IsNone())
	{
		DropButton = Cast<UButton>(WidgetTree->FindWidget(DropButtonWidgetName));
		if (DropButton)
		{
			DropButton->OnClicked.AddUniqueDynamic(this, &UPDInventoryItemContextMenuWidget::HandleDropButtonClicked);
		}
	}

	if (!EquipButton && !EquipButtonWidgetName.IsNone())
	{
		EquipButton = Cast<UButton>(WidgetTree->FindWidget(EquipButtonWidgetName));
		if (EquipButton)
		{
			EquipButton->OnClicked.AddUniqueDynamic(this, &UPDInventoryItemContextMenuWidget::HandleEquipButtonClicked);
		}
	}
}

void UPDInventoryItemContextMenuWidget::RefreshButtonVisibility()
{
	const bool bHasItem = !SlotData.IsEmpty();
	const bool bIsConsumable = bHasItem && SlotData.ItemData.ItemType == EPDItemType::Consumable;
	const bool bIsEquipment = bHasItem && SlotData.ItemData.ItemType == EPDItemType::Equipment;

	if (UseButton)
	{
		UseButton->SetVisibility(bIsConsumable ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	if (EquipButton)
	{
		EquipButton->SetVisibility(bIsEquipment ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	if (DropButton)
	{
		DropButton->SetVisibility(bHasItem ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UPDInventoryItemContextMenuWidget::HandleUseButtonClicked()
{
	OnUseClicked.Broadcast(this, SlotIndex);
}

void UPDInventoryItemContextMenuWidget::HandleDropButtonClicked()
{
	OnDropClicked.Broadcast(this, SlotIndex);
}

void UPDInventoryItemContextMenuWidget::HandleEquipButtonClicked()
{
	OnEquipClicked.Broadcast(this, SlotIndex);
}
