#include "Widgets/HUD/PDSkillSlotBarWidget.h"

#include "Engine/Texture2D.h"
#include "Widgets/HUD/PDSkillSlotWidget.h"

void UPDSkillSlotBarWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RebuildSlots();
}

void UPDSkillSlotBarWidget::SetSelectedIndex(int32 NewIndex)
{
	if (SelectedIndex == NewIndex)
	{
		return;
	}
	SelectedIndex = NewIndex;
	ApplySelection();
}

void UPDSkillSlotBarWidget::RebuildSlots()
{
	CollectSlotWidgets();

	for (int32 Index = 0; Index < SlotWidgets.Num(); ++Index)
	{
		UPDSkillSlotWidget* SlotWidget = SlotWidgets[Index];
		if (!SlotWidget)
		{
			continue;
		}

		SlotWidget->SetSlotTextures(SelectedTexture, UnselectedTexture);

		UTexture2D* Icon = SkillIcons.IsValidIndex(Index) ? SkillIcons[Index].LoadSynchronous() : nullptr;
		SlotWidget->SetSkillIcon(Icon);
	}

	ApplySelection();
}

void UPDSkillSlotBarWidget::CollectSlotWidgets()
{
	SlotWidgets.Reset();
	SlotWidgets.Add(Slot_0);
	SlotWidgets.Add(Slot_1);
	SlotWidgets.Add(Slot_2);
	SlotWidgets.Add(Slot_3);
}

void UPDSkillSlotBarWidget::ApplySelection()
{
	for (int32 Index = 0; Index < SlotWidgets.Num(); ++Index)
	{
		if (UPDSkillSlotWidget* SlotWidget = SlotWidgets[Index])
		{
			SlotWidget->SetSelected(Index == SelectedIndex);
		}
	}
}
