#include "Widgets/Quest/PDQuestListItemWidget.h"
#include "Widgets/PDWidgetSoundLibrary.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Widgets/Quest/PDQuestWindowWidget.h"

void UPDQuestListItemWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (BTN_Select)
	{
		BTN_Select->OnClicked.RemoveDynamic(this, &UPDQuestListItemWidget::HandleSelectClicked);
		BTN_Select->OnClicked.AddDynamic(this, &UPDQuestListItemWidget::HandleSelectClicked);
	}
}

void UPDQuestListItemWidget::SetupQuestItem(UPDQuestWindowWidget* InOwnerWidget, const FPDQuestData& InQuestData, EPDQuestState InState, const FText& InProgressText, bool bInSelected)
{
	OwnerWidget = InOwnerWidget;
	QuestData = InQuestData;
	QuestState = InState;
	ProgressText = InProgressText;
	bSelected = bInSelected;

	if (TXT_QuestName)
	{
		TXT_QuestName->SetText(QuestData.QuestName);
	}

	UTextBlock* ProgressTextBlock = TXT_Progress ? TXT_Progress.Get() : TXT_QuestProgress.Get();
	if (ProgressTextBlock)
	{
		ProgressTextBlock->SetText(ProgressText);
	}

	UpdateVisualStyle();
}

void UPDQuestListItemWidget::SetSelected(bool bInSelected)
{
	bSelected = bInSelected;
	UpdateVisualStyle();
}

void UPDQuestListItemWidget::UpdateVisualStyle()
{
	if (!BTN_Select)
	{
		return;
	}

	const FLinearColor SelectedColor(0.15f, 0.85f, 0.15f, 1.0f);
	const FLinearColor NormalColor(0.02f, 0.02f, 0.02f, 0.85f);
	BTN_Select->SetBackgroundColor(bSelected ? SelectedColor : NormalColor);
}

void UPDQuestListItemWidget::HandleSelectClicked()
{
	UPDWidgetSoundLibrary::PlayUISound2D(this, ButtonClickSound);

	if (OwnerWidget && !QuestData.QuestID.IsNone())
	{
		OwnerWidget->SelectQuest(QuestData.QuestID);
	}
}
