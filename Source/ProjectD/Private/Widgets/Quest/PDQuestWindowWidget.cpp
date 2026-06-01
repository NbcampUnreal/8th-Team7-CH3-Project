#include "Widgets/Quest/PDQuestWindowWidget.h"
#include "Widgets/PDWidgetSoundLibrary.h"

#include "Blueprint/WidgetTree.h"
#include "Components/ActorComponent.h"
#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Core/PDPlayerComponentResolver.h"
#include "Core/PDPlayerController.h"
#include "Data/PDQuestComponent.h"
#include "GameFramework/Pawn.h"
#include "Items/Containers/PDInventoryComponent.h"
#include "Widgets/Quest/PDQuestListItemWidget.h"

void UPDQuestWindowWidget::InitializeForOwner(APlayerController* /*OwnerPC*/)
{
}

void UPDQuestWindowWidget::OnTabShown()
{
	RefreshQuestList();
	RefreshQuestDetail();
}

void UPDQuestWindowWidget::OnTabHidden()
{
}

void UPDQuestWindowWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BindButtons();

	if (!QuestComponent)
	{
		QuestComponent = FindQuestComponent();
	}

	if (!InventoryComponent)
	{
		InventoryComponent = FindInventoryComponent();
	}

	BindQuestComponent();
	RefreshQuestList();
	RefreshQuestDetail();
}

void UPDQuestWindowWidget::NativeDestruct()
{
	UnbindQuestComponent();
	Super::NativeDestruct();
}

void UPDQuestWindowWidget::InitializeQuestWindow(UPDQuestComponent* InQuestComponent, UPDInventoryComponent* InInventoryComponent)
{
	UnbindQuestComponent();
	QuestComponent = InQuestComponent;
	InventoryComponent = InInventoryComponent;
	BindQuestComponent();
	RefreshQuestList();
	RefreshQuestDetail();
}

void UPDQuestWindowWidget::RefreshQuestList()
{
	UpdateTabButtonStyle();

	if (!SB_QuestList)
	{
		return;
	}

	SB_QuestList->ClearChildren();

	if (!QuestComponent || !QuestListItemClass)
	{
		return;
	}

	if (CurrentTabState == EPDQuestState::Inactive)
	{
		const TArray<FPDQuestData> AvailableQuests = QuestComponent->GetAvailableQuests();
		for (const FPDQuestData& QuestData : AvailableQuests)
		{
			UPDQuestListItemWidget* ItemWidget = CreateWidget<UPDQuestListItemWidget>(GetOwningPlayer(), QuestListItemClass);
			if (ItemWidget)
			{
				ItemWidget->SetupQuestItem(this, QuestData, EPDQuestState::Inactive, MakeProgressText(QuestData, nullptr), SelectedQuestID == QuestData.QuestID);
				SB_QuestList->AddChild(ItemWidget);
			}
		}
		return;
	}

	const TArray<FPDQuestProgress> QuestProgresses = QuestComponent->GetQuestsByState(CurrentTabState);
	for (const FPDQuestProgress& QuestProgress : QuestProgresses)
	{
		UPDQuestListItemWidget* ItemWidget = CreateWidget<UPDQuestListItemWidget>(GetOwningPlayer(), QuestListItemClass);
		if (ItemWidget)
		{
			ItemWidget->SetupQuestItem(this, QuestProgress.QuestData, QuestProgress.State, MakeProgressText(QuestProgress.QuestData, &QuestProgress), SelectedQuestID == QuestProgress.QuestData.QuestID);
			SB_QuestList->AddChild(ItemWidget);
		}
	}
}

void UPDQuestWindowWidget::RefreshQuestDetail()
{
	FPDQuestData QuestData;
	FPDQuestProgress QuestProgress;
	bool bHasProgress = false;

	if (!GetSelectedQuestData(QuestData, QuestProgress, bHasProgress))
	{
		ClearDetail();
		return;
	}

	if (TXT_QuestTitle)
	{
		TXT_QuestTitle->SetText(QuestData.QuestName);
	}

	if (TXT_QuestDescription)
	{
		TXT_QuestDescription->SetText(QuestData.Description);
	}

	if (VB_Objectives)
	{
		VB_Objectives->ClearChildren();
		for (const FPDQuestObjective& Objective : QuestData.Objectives)
		{
			const int32 Current = bHasProgress ? QuestProgress.ObjectiveProgress.FindRef(Objective.GetProgressKey()) : 0;
			const int32 Required = FMath::Max(1, Objective.RequiredCount);
			const FText LineText = FText::FromString(FString::Printf(TEXT("%s %d/%d"), *Objective.Description.ToString(), Current, Required));
			AddDetailLine(VB_Objectives, LineText);
		}
	}

	if (VB_Rewards)
	{
		VB_Rewards->ClearChildren();
		if (QuestData.Reward.RewardGold > 0)
		{
			AddDetailLine(VB_Rewards, FText::FromString(FString::Printf(TEXT("Gold +%d"), QuestData.Reward.RewardGold)));
		}

		if (QuestData.Reward.RewardTraderReputationExp > 0)
		{
			AddDetailLine(VB_Rewards, FText::FromString(FString::Printf(TEXT("Market EXP +%d"), QuestData.Reward.RewardTraderReputationExp)));
		}

		for (const FPDQuestRewardItem& RewardItem : QuestData.Reward.RewardItems)
		{
			if (RewardItem.ItemID.IsNone())
			{
				continue;
			}

			FText RewardText = FText::FromName(RewardItem.ItemID);
			if (InventoryComponent)
			{
				FPDItemData ItemData;
				if (InventoryComponent->FindItemDataByID(RewardItem.ItemID, ItemData) && !ItemData.DisplayName.IsEmpty())
				{
					RewardText = ItemData.DisplayName;
				}
			}

			const int32 Quantity = FMath::Max(1, RewardItem.Quantity);
			AddDetailLine(VB_Rewards, Quantity > 1
				? FText::FromString(FString::Printf(TEXT("%s x%d"), *RewardText.ToString(), Quantity))
				: RewardText);
		}
	}

	const EPDQuestState State = bHasProgress ? QuestProgress.State : EPDQuestState::Inactive;

	if (TXT_ActionButton)
	{
		TXT_ActionButton->SetText(GetActionButtonText(State));
	}

	if (BTN_Action)
	{
		BTN_Action->SetIsEnabled(State == EPDQuestState::Inactive || State == EPDQuestState::Completed);
	}
}

void UPDQuestWindowWidget::SelectQuest(FName QuestID)
{
	SelectedQuestID = QuestID;
	RefreshQuestList();
	RefreshQuestDetail();
}

void UPDQuestWindowWidget::SetTabState(EPDQuestState NewState)
{
	CurrentTabState = NewState;
	SelectedQuestID = NAME_None;
	UpdateTabButtonStyle();
	RefreshQuestList();
	RefreshQuestDetail();
}

void UPDQuestWindowWidget::HandleQuestUpdated()
{
	RefreshQuestList();
	RefreshQuestDetail();
}

void UPDQuestWindowWidget::HandleAvailableClicked()
{
	UPDWidgetSoundLibrary::PlayUISound2D(this, ButtonClickSound);

	SetTabState(EPDQuestState::Inactive);
}

void UPDQuestWindowWidget::HandleActiveClicked()
{
	UPDWidgetSoundLibrary::PlayUISound2D(this, ButtonClickSound);

	SetTabState(EPDQuestState::Active);
}

void UPDQuestWindowWidget::HandleCompletedClicked()
{
	UPDWidgetSoundLibrary::PlayUISound2D(this, ButtonClickSound);

	SetTabState(EPDQuestState::Completed);
}

void UPDQuestWindowWidget::HandleActionClicked()
{
	UPDWidgetSoundLibrary::PlayUISound2D(this, ButtonClickSound);

	if (!QuestComponent || SelectedQuestID.IsNone())
	{
		return;
	}

	FPDQuestProgress QuestProgress;
	if (QuestComponent->GetQuestProgress(SelectedQuestID, QuestProgress))
	{
		if (QuestProgress.State == EPDQuestState::Completed)
		{
			UPDInventoryComponent* Inventory = InventoryComponent ? InventoryComponent.Get() : FindInventoryComponent();
			QuestComponent->GiveReward(SelectedQuestID, Inventory);
		}
		return;
	}

	if (CurrentTabState == EPDQuestState::Inactive)
	{
		QuestComponent->AddQuestByID(SelectedQuestID);
		SetTabState(EPDQuestState::Active);
	}
}

void UPDQuestWindowWidget::BindQuestComponent()
{
	if (QuestComponent)
	{
		QuestComponent->OnQuestUpdated.RemoveDynamic(this, &UPDQuestWindowWidget::HandleQuestUpdated);
		QuestComponent->OnQuestUpdated.AddDynamic(this, &UPDQuestWindowWidget::HandleQuestUpdated);
	}
}

void UPDQuestWindowWidget::UnbindQuestComponent()
{
	if (QuestComponent)
	{
		QuestComponent->OnQuestUpdated.RemoveDynamic(this, &UPDQuestWindowWidget::HandleQuestUpdated);
	}
}

void UPDQuestWindowWidget::BindButtons()
{
	if (BTN_Available)
	{
		BTN_Available->OnClicked.RemoveDynamic(this, &UPDQuestWindowWidget::HandleAvailableClicked);
		BTN_Available->OnClicked.AddDynamic(this, &UPDQuestWindowWidget::HandleAvailableClicked);
	}

	if (BTN_Active)
	{
		BTN_Active->OnClicked.RemoveDynamic(this, &UPDQuestWindowWidget::HandleActiveClicked);
		BTN_Active->OnClicked.AddDynamic(this, &UPDQuestWindowWidget::HandleActiveClicked);
	}

	if (BTN_Completed)
	{
		BTN_Completed->OnClicked.RemoveDynamic(this, &UPDQuestWindowWidget::HandleCompletedClicked);
		BTN_Completed->OnClicked.AddDynamic(this, &UPDQuestWindowWidget::HandleCompletedClicked);
	}

	if (BTN_Action)
	{
		BTN_Action->OnClicked.RemoveDynamic(this, &UPDQuestWindowWidget::HandleActionClicked);
		BTN_Action->OnClicked.AddDynamic(this, &UPDQuestWindowWidget::HandleActionClicked);
	}
}

void UPDQuestWindowWidget::ClearDetail()
{
	if (TXT_QuestTitle)
	{
		TXT_QuestTitle->SetText(FText::FromString(TEXT("Select Quest")));
	}

	if (TXT_QuestDescription)
	{
		TXT_QuestDescription->SetText(FText::FromString(TEXT("Select a quest from the left.")));
	}

	if (VB_Objectives)
	{
		VB_Objectives->ClearChildren();
	}

	if (VB_Rewards)
	{
		VB_Rewards->ClearChildren();
	}

	if (TXT_ActionButton)
	{
		TXT_ActionButton->SetText(FText::FromString(TEXT("Accept")));
	}

	if (BTN_Action)
	{
		BTN_Action->SetIsEnabled(false);
	}
}

bool UPDQuestWindowWidget::GetSelectedQuestData(FPDQuestData& OutQuestData, FPDQuestProgress& OutQuestProgress, bool& bOutHasProgress) const
{
	bOutHasProgress = false;

	if (!QuestComponent || SelectedQuestID.IsNone())
	{
		return false;
	}

	if (QuestComponent->GetQuestProgress(SelectedQuestID, OutQuestProgress))
	{
		OutQuestData = OutQuestProgress.QuestData;
		bOutHasProgress = true;
		return true;
	}

	for (const FPDQuestData& QuestData : QuestComponent->GetAvailableQuests())
	{
		if (QuestData.QuestID == SelectedQuestID)
		{
			OutQuestData = QuestData;
			return true;
		}
	}

	return false;
}

UPDQuestComponent* UPDQuestWindowWidget::FindQuestComponent() const
{
	if (UPDQuestComponent* Quest = FPDPlayerComponentResolver::ResolveQuest(GetOwningPlayer()))
	{
		return Quest;
	}
	return FPDPlayerComponentResolver::ResolveQuest(GetOwningPlayerPawn());
}

UPDInventoryComponent* UPDQuestWindowWidget::FindInventoryComponent() const
{
	if (UPDInventoryComponent* Inventory = FPDPlayerComponentResolver::ResolveInventory(GetOwningPlayer()))
	{
		return Inventory;
	}
	return FPDPlayerComponentResolver::ResolveInventory(GetOwningPlayerPawn());
}

void UPDQuestWindowWidget::AddDetailLine(UVerticalBox* TargetBox, const FText& Text) const
{
	if (!TargetBox || !WidgetTree)
	{
		return;
	}

	UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	if (TextBlock)
	{
		TextBlock->SetText(Text);
		TextBlock->SetAutoWrapText(true);
		TargetBox->AddChildToVerticalBox(TextBlock);
	}
}

void UPDQuestWindowWidget::UpdateTabButtonStyle() const
{
	ApplyTabButtonStyle(BTN_Available, CurrentTabState == EPDQuestState::Inactive);
	ApplyTabButtonStyle(BTN_Active, CurrentTabState == EPDQuestState::Active);
	ApplyTabButtonStyle(BTN_Completed, CurrentTabState == EPDQuestState::Completed);
}

void UPDQuestWindowWidget::ApplyTabButtonStyle(UButton* TargetButton, bool bSelected) const
{
	if (!TargetButton)
	{
		return;
	}

	const FLinearColor SelectedColor(0.15f, 0.85f, 0.15f, 1.0f);
	const FLinearColor NormalColor(0.02f, 0.02f, 0.02f, 0.85f);
	TargetButton->SetBackgroundColor(bSelected ? SelectedColor : NormalColor);
}

FText UPDQuestWindowWidget::MakeProgressText(const FPDQuestData& QuestData, const FPDQuestProgress* QuestProgress) const
{
	if (QuestProgress && QuestProgress->State == EPDQuestState::Rewarded)
	{
		return FText::FromString(TEXT("Completed"));
	}

	int32 Current = 0;
	int32 Required = 0;
	GetObjectiveTotalCounts(QuestData, QuestProgress, Current, Required);

	if (Required <= 0)
	{
		return QuestProgress && QuestProgress->State == EPDQuestState::Completed ? FText::FromString(TEXT("Completed")) : FText::FromString(TEXT("0 / 0"));
	}

	return FText::FromString(FString::Printf(TEXT("%d / %d"), Current, Required));
}

void UPDQuestWindowWidget::GetObjectiveTotalCounts(const FPDQuestData& QuestData, const FPDQuestProgress* QuestProgress, int32& OutCurrent, int32& OutRequired) const
{
	OutCurrent = 0;
	OutRequired = 0;

	for (const FPDQuestObjective& Objective : QuestData.Objectives)
	{
		const int32 Required = FMath::Max(1, Objective.RequiredCount);
		const int32 Current = QuestProgress ? QuestProgress->ObjectiveProgress.FindRef(Objective.GetProgressKey()) : 0;
		OutCurrent += FMath::Clamp(Current, 0, Required);
		OutRequired += Required;
	}
}

FText UPDQuestWindowWidget::GetActionButtonText(EPDQuestState State) const
{
	switch (State)
	{
	case EPDQuestState::Inactive:
		return FText::FromString(TEXT("Accept"));
	case EPDQuestState::Active:
		return FText::FromString(TEXT("In Progress"));
	case EPDQuestState::Completed:
		return FText::FromString(TEXT("Claim Reward"));
	case EPDQuestState::Rewarded:
		return FText::FromString(TEXT("Completed"));
	default:
		return FText::GetEmpty();
	}
}
