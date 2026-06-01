#include "Widgets/Inventory/PDEquipmentListItemWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Input/Events.h"

void UPDEquipmentListItemWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	RefreshVisuals();
}

FReply UPDEquipmentListItemWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		OnItemClicked.Broadcast(this, SlotIndex);
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UPDEquipmentListItemWidget::SetSlotData(const FPDInventorySlot& InSlotData, int32 InSlotIndex)
{
	SlotData = InSlotData;
	SlotIndex = InSlotIndex;
	RefreshVisuals();
}

void UPDEquipmentListItemWidget::SetSelected(bool bInSelected)
{
	bSelected = bInSelected;
	SetRenderOpacity(bSelected ? 1.f : 0.85f);
}

void UPDEquipmentListItemWidget::RefreshVisuals()
{
	if (SlotData.IsEmpty())
	{
		if (Image_ItemIcon)
		{
			Image_ItemIcon->SetBrushFromTexture(nullptr);
			Image_ItemIcon->SetVisibility(ESlateVisibility::Hidden);
		}

		if (Text_ItemName)
		{
			Text_ItemName->SetText(FText::GetEmpty());
		}

		if (Text_Level)
		{
			Text_Level->SetText(FText::GetEmpty());
		}

		return;
	}

	const FPDItemData& ItemData = SlotData.ItemData;
	const FText DisplayName = ItemData.DisplayName.IsEmpty() ? FText::FromName(ItemData.ItemID) : ItemData.DisplayName;

	if (Image_ItemIcon)
	{
		Image_ItemIcon->SetBrushFromTexture(ItemData.Icon, true);
		Image_ItemIcon->SetVisibility(ItemData.Icon ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}

	if (Text_ItemName)
	{
		Text_ItemName->SetText(FText::FromString(FString::Printf(TEXT("ITEM : %s"), *DisplayName.ToString())));
	}

	if (Text_Level)
	{
		Text_Level->SetText(FText::FromString(FString::Printf(TEXT("Modification : +%d"), FMath::Max(0, SlotData.ModificationLevel))));
	}
}
