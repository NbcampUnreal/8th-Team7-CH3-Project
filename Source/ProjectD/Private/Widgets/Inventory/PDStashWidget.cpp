#include "Widgets/Inventory/PDStashWidget.h"
#include "Widgets/PDWidgetSoundLibrary.h"

#include "Algo/Sort.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Characters/PDPlayerCharacter.h"
#include "Components/Button.h"
#include "Core/PDPlayerComponentResolver.h"
#include "Core/PDPlayerController.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Components/UniformGridPanel.h"
#include "Components/SizeBox.h"
#include "Components/SizeBoxSlot.h"
#include "Components/UniformGridSlot.h"
#include "GameFramework/Pawn.h"
<<<<<<< HEAD
#include "Items/Containers/PDInventoryComponent.h"
#include "Items/Data/PDItemSlotTransfer.h"
#include "Items/Containers/PDQuickSlotComponent.h"
#include "Items/Containers/PDStashComponent.h"
#include "Weapons/Base/PDWeaponBase.h"
=======
#include "Items/PDInventoryComponent.h"
#include "Items/PDItemSlotTransfer.h"
#include "Items/PDQuickSlotComponent.h"
#include "Items/PDStashComponent.h"
>>>>>>> origin/main
#include "Widgets/Inventory/PDInventorySlotWidget.h"
#include "Widgets/Inventory/PDQuantityPopupWidget.h"

void UPDStashWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BindStashChanged();
	BindTabButtons();
	BindSortButtons();
	CacheFilterTabBaseLabels();
	if (Button_UpgradeStash)
	{
		Button_UpgradeStash->OnClicked.AddUniqueDynamic(this, &UPDStashWidget::HandleUpgradeStashClicked);
	}
	SetSortOptionsVisible(false);
	RefreshStashGrid();
	UpdateTabButtonStyle();
}

void UPDStashWidget::NativeDestruct()
{
	if (ActiveQuantityPopup && ActiveQuantityPopup->IsInViewport())
	{
		ActiveQuantityPopup->RemoveFromParent();
	}
	ClearQuantityRequest();
	UnbindStashChanged();
	Super::NativeDestruct();
}

void UPDStashWidget::RefreshStashGrid()
{
	ResolveStashGridPanel();

	if (!StashGridPanel)
	{
		UE_LOG(LogTemp, Warning, TEXT("PDStashWidget: Stash grid widget was not found. Expected widget name: %s"), *StashGridWidgetName.ToString());
		return;
	}

	StashGridPanel->ClearChildren();


	StashGridPanel->SetSlotPadding(FMargin(0.f));
	StashGridPanel->SetMinDesiredSlotWidth(StashSlotWidth);
	StashGridPanel->SetMinDesiredSlotHeight(StashSlotHeight);

	if (!StashSlotWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("PDStashWidget: StashSlotWidgetClass is not set. Set it to WBP_InventorySlot in WBP_Stash Class Defaults."));
		return;
	}

	UPDStashComponent* StashComponent = FindStashComponent();


	const int32 Columns = FMath::Max(1, StashComponent ? StashComponent->GridColumns : FallbackGridColumns);
	const int32 Rows = FMath::Max(1, StashComponent ? StashComponent->GridRows : FallbackGridRows);
	const int32 SlotCount = Columns * Rows;

	TArray<int32> DisplaySlotIndices;
	if (StashComponent)
	{


		for (int32 RealSlotIndex = 0; RealSlotIndex < StashComponent->StashItems.Num(); ++RealSlotIndex)
		{
			const FPDInventorySlot& StashSlotData = StashComponent->StashItems[RealSlotIndex];
			if (!StashSlotData.IsEmpty() && DoesSlotMatchCurrentFilter(StashSlotData))
			{
				DisplaySlotIndices.Add(RealSlotIndex);
			}
		}

		SortDisplaySlotIndices(DisplaySlotIndices, StashComponent);

		for (int32 RealSlotIndex = 0; RealSlotIndex < StashComponent->StashItems.Num(); ++RealSlotIndex)
		{
			const FPDInventorySlot& StashSlotData = StashComponent->StashItems[RealSlotIndex];
			if (StashSlotData.IsEmpty())
			{
				DisplaySlotIndices.Add(RealSlotIndex);
			}
		}
	}

	for (int32 DisplayIndex = 0; DisplayIndex < SlotCount; ++DisplayIndex)
	{
		UUserWidget* CreatedSlotWidget = CreateWidget<UUserWidget>(GetOwningPlayer(), StashSlotWidgetClass);
		if (!CreatedSlotWidget)
		{
			continue;
		}

		if (UPDInventorySlotWidget* StashSlotWidget = Cast<UPDInventorySlotWidget>(CreatedSlotWidget))
		{
			StashSlotWidget->SetSlotContainerType(EPDItemContainerType::Stash);
			StashSlotWidget->OnSlotLeftClicked.AddUniqueDynamic(this, &UPDStashWidget::HandleStashSlotLeftClicked);
			StashSlotWidget->OnSlotItemDropped.AddUniqueDynamic(this, &UPDStashWidget::HandleStashSlotItemDropped);

			if (StashComponent && DisplaySlotIndices.IsValidIndex(DisplayIndex))
			{
				const int32 RealSlotIndex = DisplaySlotIndices[DisplayIndex];
				if (StashComponent->StashItems.IsValidIndex(RealSlotIndex) && !StashComponent->StashItems[RealSlotIndex].IsEmpty())
				{
					StashSlotWidget->SetSlotData(StashComponent->StashItems[RealSlotIndex], RealSlotIndex);
				}
				else
				{
					StashSlotWidget->ClearSlotData(RealSlotIndex);
				}
			}
			else
			{
				StashSlotWidget->ClearSlotData(INDEX_NONE);
			}
		}

		USizeBox* SlotSizeBox = WidgetTree ? WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass()) : nullptr;
		if (SlotSizeBox)
		{
			SlotSizeBox->SetWidthOverride(StashSlotWidth);
			SlotSizeBox->SetHeightOverride(StashSlotHeight);
			SlotSizeBox->SetMinDesiredWidth(StashSlotWidth);
			SlotSizeBox->SetMinDesiredHeight(StashSlotHeight);

			if (USizeBoxSlot* SizeBoxSlot = Cast<USizeBoxSlot>(SlotSizeBox->AddChild(CreatedSlotWidget)))
			{

				SizeBoxSlot->SetHorizontalAlignment(HAlign_Fill);
				SizeBoxSlot->SetVerticalAlignment(VAlign_Fill);
				SizeBoxSlot->SetPadding(FMargin(0.f));
			}
		}

		UWidget* GridChildWidget = SlotSizeBox ? static_cast<UWidget*>(SlotSizeBox) : static_cast<UWidget*>(CreatedSlotWidget);
		UUniformGridSlot* GridSlot = StashGridPanel->AddChildToUniformGrid(GridChildWidget, DisplayIndex / Columns, DisplayIndex % Columns);
		if (GridSlot)
		{


			GridSlot->SetHorizontalAlignment(HAlign_Left);
			GridSlot->SetVerticalAlignment(VAlign_Top);
		}
	}

	UpdateTabButtonStyle();
}

void UPDStashWidget::SetStashFilterTab(EPDItemFilterTab NewFilterTab)
{
	if (CurrentFilterTab == NewFilterTab)
	{
		UpdateTabButtonStyle();
		return;
	}

	CurrentFilterTab = NewFilterTab;
	CurrentSortMode = EPDItemSortMode::None;
	SetSortOptionsVisible(false);
	RefreshStashGrid();
}

void UPDStashWidget::SetStashSortMode(EPDItemSortMode NewSortMode)
{
	if (CurrentSortMode == NewSortMode)
	{
		SetSortOptionsVisible(false);
		return;
	}

	CurrentSortMode = NewSortMode;
	SetSortOptionsVisible(false);
	RefreshStashGrid();
}

void UPDStashWidget::BindTabButtons()
{
	if (Button_Equipment)
	{
		Button_Equipment->OnClicked.AddUniqueDynamic(this, &UPDStashWidget::HandleEquipmentTabClicked);
	}

	if (Button_Consumable)
	{
		Button_Consumable->OnClicked.AddUniqueDynamic(this, &UPDStashWidget::HandleConsumableTabClicked);
	}

	if (Button_Misc)
	{
		Button_Misc->OnClicked.AddUniqueDynamic(this, &UPDStashWidget::HandleMiscTabClicked);
	}
}

void UPDStashWidget::BindSortButtons()
{
	if (Button_Sort)
	{
		Button_Sort->OnClicked.AddUniqueDynamic(this, &UPDStashWidget::HandleSortButtonClicked);
	}

	if (Button_SortByName)
	{
		Button_SortByName->OnClicked.AddUniqueDynamic(this, &UPDStashWidget::HandleSortByNameClicked);
	}

	if (Button_SortByType)
	{
		Button_SortByType->OnClicked.AddUniqueDynamic(this, &UPDStashWidget::HandleSortByTypeClicked);
	}

	if (Button_SortTab_Name)
	{
		Button_SortTab_Name->OnClicked.AddUniqueDynamic(this, &UPDStashWidget::HandleSortByNameClicked);
	}

	if (Button_SortTab_Type)
	{
		Button_SortTab_Type->OnClicked.AddUniqueDynamic(this, &UPDStashWidget::HandleSortByTypeClicked);
	}
}

void UPDStashWidget::UpdateTabButtonStyle()
{
	if (FilterTabBaseLabels.IsEmpty())
	{
		CacheFilterTabBaseLabels();
	}

	const FLinearColor SelectedColor(0.15f, 0.85f, 0.15f, 1.0f);
	const FLinearColor NormalColor(0.02f, 0.02f, 0.02f, 0.85f);

	const int32 MaxSlots = GetStashDisplaySlotCount();
	const int32 EquipmentUsedSlots = CountOccupiedStashSlotsByType(EPDItemType::Equipment);
	const int32 ConsumableUsedSlots = CountOccupiedStashSlotsByType(EPDItemType::Consumable);
	const int32 MiscUsedSlots = CountOccupiedStashSlotsByType(EPDItemType::Misc);

	if (Button_Equipment)
	{
		Button_Equipment->SetBackgroundColor(CurrentFilterTab == EPDItemFilterTab::Equipment ? SelectedColor : NormalColor);
		SetTabButtonLabel(Button_Equipment, GetFilterTabBaseLabel(EPDItemFilterTab::Equipment), EquipmentUsedSlots, MaxSlots);
	}

	if (Button_Consumable)
	{
		Button_Consumable->SetBackgroundColor(CurrentFilterTab == EPDItemFilterTab::Consumable ? SelectedColor : NormalColor);
		SetTabButtonLabel(Button_Consumable, GetFilterTabBaseLabel(EPDItemFilterTab::Consumable), ConsumableUsedSlots, MaxSlots);
	}

	if (Button_Misc)
	{
		Button_Misc->SetBackgroundColor(CurrentFilterTab == EPDItemFilterTab::Misc ? SelectedColor : NormalColor);
		SetTabButtonLabel(Button_Misc, GetFilterTabBaseLabel(EPDItemFilterTab::Misc), MiscUsedSlots, MaxSlots);
	}
}

int32 UPDStashWidget::CountOccupiedStashSlotsByType(EPDItemType ItemType) const
{
	const UPDStashComponent* StashComponent = FindStashComponent();
	if (!StashComponent)
	{
		return 0;
	}

	int32 UsedSlotCount = 0;
	for (const FPDInventorySlot& StashSlotData : StashComponent->StashItems)
	{
		if (!StashSlotData.IsEmpty() && StashSlotData.ItemData.ItemType == ItemType)
		{
			++UsedSlotCount;
		}
	}

	return UsedSlotCount;
}

int32 UPDStashWidget::GetStashDisplaySlotCount() const
{
	const UPDStashComponent* StashComponent = FindStashComponent();
	const int32 Columns = FMath::Max(1, StashComponent ? StashComponent->GridColumns : FallbackGridColumns);
	const int32 Rows = FMath::Max(1, StashComponent ? StashComponent->GridRows : FallbackGridRows);
	return Columns * Rows;
}


void UPDStashWidget::CacheFilterTabBaseLabels()
{
	FilterTabBaseLabels.Reset();
	CacheFilterTabBaseLabel(EPDItemFilterTab::Equipment, Button_Equipment);
	CacheFilterTabBaseLabel(EPDItemFilterTab::Consumable, Button_Consumable);
	CacheFilterTabBaseLabel(EPDItemFilterTab::Misc, Button_Misc);
}

void UPDStashWidget::CacheFilterTabBaseLabel(EPDItemFilterTab FilterTab, UButton* TargetButton)
{
	if (UTextBlock* ButtonText = GetTabButtonTextBlock(TargetButton))
	{
		FString LabelString = ButtonText->GetText().ToString();
		int32 CountStartIndex = INDEX_NONE;
		if (LabelString.FindLastChar(TEXT('('), CountStartIndex))
		{
			LabelString = LabelString.Left(CountStartIndex).TrimEnd();
		}

		if (!LabelString.IsEmpty())
		{
			FilterTabBaseLabels.Add(FilterTab, FText::FromString(LabelString));
		}
	}
}

FText UPDStashWidget::GetFilterTabBaseLabel(EPDItemFilterTab FilterTab) const
{
	if (const FText* BaseLabel = FilterTabBaseLabels.Find(FilterTab))
	{
		return *BaseLabel;
	}

	return FText::GetEmpty();
}

UTextBlock* UPDStashWidget::GetTabButtonTextBlock(UButton* TargetButton) const
{
	return TargetButton ? Cast<UTextBlock>(TargetButton->GetContent()) : nullptr;
}

void UPDStashWidget::SetTabButtonLabel(UButton* TargetButton, const FText& BaseLabel, int32 UsedSlots, int32 MaxSlots) const
{
	if (!TargetButton)
	{
		return;
	}

	if (UTextBlock* ButtonText = GetTabButtonTextBlock(TargetButton))
	{
		ButtonText->SetText(FText::FromString(FString::Printf(TEXT("%s (%d/%d)"), *BaseLabel.ToString(), UsedSlots, FMath::Max(1, MaxSlots))));
	}
}

bool UPDStashWidget::DoesSlotMatchCurrentFilter(const FPDInventorySlot& StashSlotData) const
{
	return !StashSlotData.IsEmpty() && DoesItemTypeMatchCurrentFilter(StashSlotData.ItemData.ItemType);
}

bool UPDStashWidget::DoesItemTypeMatchCurrentFilter(EPDItemType ItemType) const
{
	if (CurrentFilterTab == EPDItemFilterTab::Equipment)
	{
		return ItemType == EPDItemType::Equipment;
	}

	if (CurrentFilterTab == EPDItemFilterTab::Consumable)
	{
		return ItemType == EPDItemType::Consumable;
	}

	if (CurrentFilterTab == EPDItemFilterTab::Misc)
	{
		return ItemType == EPDItemType::Misc;
	}

	return false;
}

bool UPDStashWidget::CanAcceptDropForCurrentFilter(const UPDInventoryDragDropOperation* DragOperation) const
{
	return DragOperation
		&& DragOperation->IsValidPayload()
		&& !DragOperation->SlotData.IsEmpty()
		&& DoesItemTypeMatchCurrentFilter(DragOperation->SlotData.ItemData.ItemType);
}

void UPDStashWidget::SortDisplaySlotIndices(TArray<int32>& DisplaySlotIndices, const UPDStashComponent* StashComponent) const
{
	if (!StashComponent || CurrentSortMode == EPDItemSortMode::None)
	{
		return;
	}

	Algo::Sort(DisplaySlotIndices, [this, StashComponent](int32 LeftIndex, int32 RightIndex)
	{
		const FPDInventorySlot& LeftSlot = StashComponent->StashItems[LeftIndex];
		const FPDInventorySlot& RightSlot = StashComponent->StashItems[RightIndex];

		if (CurrentSortMode == EPDItemSortMode::Type)
		{
			const uint8 LeftType = static_cast<uint8>(LeftSlot.ItemData.ItemType);
			const uint8 RightType = static_cast<uint8>(RightSlot.ItemData.ItemType);
			if (LeftType != RightType)
			{
				return LeftType < RightType;
			}
		}

		const FString LeftName = LeftSlot.ItemData.DisplayName.IsEmpty() ? LeftSlot.ItemData.ItemID.ToString() : LeftSlot.ItemData.DisplayName.ToString();
		const FString RightName = RightSlot.ItemData.DisplayName.IsEmpty() ? RightSlot.ItemData.ItemID.ToString() : RightSlot.ItemData.DisplayName.ToString();
		const int32 NameCompare = LeftName.Compare(RightName, ESearchCase::IgnoreCase);
		if (NameCompare != 0)
		{
			return NameCompare < 0;
		}

		return LeftIndex < RightIndex;
	});
}

void UPDStashWidget::SetSortOptionsVisible(bool bVisible)
{
	UWidget* SortPanel = Panel_SortTabs ? Panel_SortTabs.Get() : Panel_SortOptions.Get();
	if (SortPanel)
	{
		SortPanel->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UPDStashWidget::ToggleSortOptions()
{
	UWidget* SortPanel = Panel_SortTabs ? Panel_SortTabs.Get() : Panel_SortOptions.Get();
	if (SortPanel)
	{
		SetSortOptionsVisible(SortPanel->GetVisibility() != ESlateVisibility::Visible);
	}
}

void UPDStashWidget::HandleEquipmentTabClicked()
{
	UPDWidgetSoundLibrary::PlayUISound2D(this, ButtonClickSound);

	SetStashFilterTab(EPDItemFilterTab::Equipment);
}

void UPDStashWidget::HandleConsumableTabClicked()
{
	UPDWidgetSoundLibrary::PlayUISound2D(this, ButtonClickSound);

	SetStashFilterTab(EPDItemFilterTab::Consumable);
}

void UPDStashWidget::HandleMiscTabClicked()
{
	UPDWidgetSoundLibrary::PlayUISound2D(this, ButtonClickSound);

	SetStashFilterTab(EPDItemFilterTab::Misc);
}

void UPDStashWidget::HandleSortButtonClicked()
{
	UPDWidgetSoundLibrary::PlayUISound2D(this, ButtonClickSound);

	ToggleSortOptions();
}

void UPDStashWidget::HandleSortByNameClicked()
{
	UPDWidgetSoundLibrary::PlayUISound2D(this, ButtonClickSound);

	SetStashSortMode(EPDItemSortMode::Name);
}

void UPDStashWidget::HandleSortByTypeClicked()
{
	UPDWidgetSoundLibrary::PlayUISound2D(this, ButtonClickSound);

	SetStashSortMode(EPDItemSortMode::Type);
}

void UPDStashWidget::HandleUpgradeStashClicked()
{
	UPDWidgetSoundLibrary::PlayUISound2D(this, ButtonClickSound);

	RequestStashUpgrade();
}

EPDStashUpgradeResult UPDStashWidget::RequestStashUpgrade()
{
	UPDStashComponent* StashComponent = FindStashComponent();
	UPDInventoryComponent* InventoryComponent = FindInventoryComponent();

	if (!StashComponent)
	{
		BP_OnStashUpgradeFailed(EPDStashUpgradeResult::InvalidConfig);
		return EPDStashUpgradeResult::InvalidConfig;
	}

	const EPDStashUpgradeResult Result = StashComponent->UpgradeStash(InventoryComponent);
	if (Result == EPDStashUpgradeResult::Success)
	{
		RefreshStashGrid();
		UpdateTabButtonStyle();
		BP_OnStashUpgradeSucceeded(StashComponent->CurrentUpgradeLevel, StashComponent->GridRows);
	}
	else
	{
		BP_OnStashUpgradeFailed(Result);
	}

	return Result;
}

void UPDStashWidget::ResolveStashGridPanel()
{
	if (StashGridPanel)
	{
		return;
	}

	if (!WidgetTree)
	{
		return;
	}

	if (UWidget* FoundWidget = WidgetTree->FindWidget(StashGridWidgetName))
	{
		StashGridPanel = Cast<UUniformGridPanel>(FoundWidget);
	}

	if (!StashGridPanel)
	{
		WidgetTree->ForEachWidget([this](UWidget* Widget)
		{
			if (!StashGridPanel)
			{
				StashGridPanel = Cast<UUniformGridPanel>(Widget);
			}
		});
	}
}

void UPDStashWidget::InitializeStash(UPDStashComponent* InStashComponent)
{
	if (TargetStashComponent == InStashComponent)
	{
		return;
	}

	UnbindStashChanged();
	TargetStashComponent = InStashComponent;
	BindStashChanged();
	RefreshStashGrid();
}

UPDStashComponent* UPDStashWidget::FindStashComponent() const
{
	return TargetStashComponent;
}

UPDInventoryComponent* UPDStashWidget::FindInventoryComponent() const
{
	if (UPDInventoryComponent* Inventory = FPDPlayerComponentResolver::ResolveInventory(GetOwningPlayer()))
	{
		return Inventory;
	}
	return FPDPlayerComponentResolver::ResolveInventory(GetOwningPlayerPawn());
}

UPDQuickSlotComponent* UPDStashWidget::FindQuickSlotComponent() const
{
	if (UPDQuickSlotComponent* QuickSlot = FPDPlayerComponentResolver::ResolveQuickSlot(GetOwningPlayer()))
	{
		return QuickSlot;
	}
	return FPDPlayerComponentResolver::ResolveQuickSlot(GetOwningPlayerPawn());
}

const FPDInventorySlot* UPDStashWidget::FindStashSlot(int32 SlotIndex) const
{
	const UPDStashComponent* StashComponent = FindStashComponent();
	return StashComponent && StashComponent->StashItems.IsValidIndex(SlotIndex) ? &StashComponent->StashItems[SlotIndex] : nullptr;
}

const FPDInventorySlot* UPDStashWidget::FindSourceSlot(EPDItemContainerType SourceContainerType, int32 SlotIndex) const
{
	switch (SourceContainerType)
	{
	case EPDItemContainerType::Inventory:
		if (const UPDInventoryComponent* InventoryComponent = FindInventoryComponent())
		{
			return InventoryComponent->Items.IsValidIndex(SlotIndex) ? &InventoryComponent->Items[SlotIndex] : nullptr;
		}
		return nullptr;
	case EPDItemContainerType::Stash:
		return FindStashSlot(SlotIndex);
	case EPDItemContainerType::QuickSlot:
		if (const UPDQuickSlotComponent* QuickSlotComponent = FindQuickSlotComponent())
		{
			return QuickSlotComponent->GetResolvedQuickSlot(SlotIndex);
		}
		return nullptr;
	default:
		return nullptr;
	}
}

UPDQuickSlotComponent* UPDStashWidget::FindQuickSlotComponent() const
{
	if (APawn* OwningPawn = GetOwningPlayerPawn())
	{
		return OwningPawn->FindComponentByClass<UPDQuickSlotComponent>();
	}

	return nullptr;
}

const FPDInventorySlot* UPDStashWidget::FindStashSlot(int32 SlotIndex) const
{
	const UPDStashComponent* StashComponent = FindStashComponent();
	return StashComponent && StashComponent->StashItems.IsValidIndex(SlotIndex) ? &StashComponent->StashItems[SlotIndex] : nullptr;
}

const FPDInventorySlot* UPDStashWidget::FindSourceSlot(EPDItemContainerType SourceContainerType, int32 SlotIndex) const
{
	switch (SourceContainerType)
	{
	case EPDItemContainerType::Inventory:
		if (const UPDInventoryComponent* InventoryComponent = FindInventoryComponent())
		{
			return InventoryComponent->Items.IsValidIndex(SlotIndex) ? &InventoryComponent->Items[SlotIndex] : nullptr;
		}
		return nullptr;
	case EPDItemContainerType::Stash:
		return FindStashSlot(SlotIndex);
	case EPDItemContainerType::QuickSlot:
		if (const UPDQuickSlotComponent* QuickSlotComponent = FindQuickSlotComponent())
		{
			return QuickSlotComponent->QuickSlotItems.IsValidIndex(SlotIndex) ? &QuickSlotComponent->QuickSlotItems[SlotIndex] : nullptr;
		}
		return nullptr;
	default:
		return nullptr;
	}
}

void UPDStashWidget::HandleStashSlotLeftClicked(UPDInventorySlotWidget* SlotWidget, int32 ClickedSlotIndex)
{
	if (!SlotWidget || !SlotWidget->WasLastClickWithControl() || SlotWidget->GetSlotData().IsEmpty())
	{
		return;
	}

	const FPDInventorySlot& SlotData = SlotWidget->GetSlotData();
	if (SlotData.Quantity <= 1 || FMath::Max(1, SlotData.ItemData.MaxStack) <= 1)
	{
		TakeStashSlotQuantity(ClickedSlotIndex, 1);
		return;
	}

	const FText DisplayName = SlotData.ItemData.DisplayName.IsEmpty() ? FText::FromName(SlotData.ItemData.ItemID) : SlotData.ItemData.DisplayName;
	OpenQuantityPopup(ClickedSlotIndex, SlotData.Quantity, DisplayName);
}

void UPDStashWidget::HandleStashSlotItemDropped(UPDInventorySlotWidget* SlotWidget, int32 TargetSlotIndex, UPDInventoryDragDropOperation* DragOperation)
{
<<<<<<< HEAD
	if (!SlotWidget || !CanAcceptDropForCurrentFilter(DragOperation) || TargetSlotIndex == INDEX_NONE)
	{
		return;
	}


	if (DragOperation->SourceContainerType == EPDItemContainerType::QuickSlot)
=======
	if (!SlotWidget || !DragOperation || !DragOperation->IsValidPayload())
>>>>>>> origin/main
	{
		return;
	}

	if (DragOperation->SourceContainerType == EPDItemContainerType::Stash)
	{
		ExecuteStashSlotTransfer(DragOperation->SourceContainerType, DragOperation->SourceSlotIndex, TargetSlotIndex, DragOperation->SlotData.Quantity);
		return;
	}

	int32 MaxQuantity = 0;
	FText Title;
	if (ShouldOpenTransferQuantityPopup(DragOperation->SourceContainerType, DragOperation->SourceSlotIndex, TargetSlotIndex, MaxQuantity, Title))
	{
		OpenTransferQuantityPopup(DragOperation->SourceContainerType, DragOperation->SourceSlotIndex, TargetSlotIndex, MaxQuantity, Title);
		return;
	}

	ExecuteStashSlotTransfer(DragOperation->SourceContainerType, DragOperation->SourceSlotIndex, TargetSlotIndex, DragOperation->SlotData.Quantity);
}

void UPDStashWidget::TakeStashSlotQuantity(int32 SlotIndex, int32 Quantity)
{
	UPDInventoryComponent* InventoryComponent = FindInventoryComponent();
	UPDStashComponent* StashComponent = FindStashComponent();

	if (!InventoryComponent || !StashComponent || Quantity <= 0)
	{
		return;
	}

	if (APDPlayerController* PlayerController = Cast<APDPlayerController>(GetOwningPlayer()))
	{
		const AActor* StashOwner = StashComponent->GetOwner();
		if (!StashOwner || !StashOwner->HasAuthority())
		{
			PlayerController->ServerTakeStashSlotQuantity(StashComponent, SlotIndex, Quantity);
			return;
		}
	}

	StashComponent->TakeStashSlotQuantity(InventoryComponent, SlotIndex, Quantity);
}


void UPDStashWidget::ExecuteStashSlotTransfer(EPDItemContainerType SourceContainerType, int32 SourceSlotIndex, int32 TargetSlotIndex, int32 Quantity)
{
	if (Quantity <= 0)
	{
		return;
	}

	UPDStashComponent* StashComponent = FindStashComponent();
	if (!StashComponent)
	{
		return;
	}

	switch (SourceContainerType)
	{
	case EPDItemContainerType::Inventory:
		if (UPDInventoryComponent* InventoryComponent = FindInventoryComponent())
		{
<<<<<<< HEAD
			if (APDPlayerController* PlayerController = Cast<APDPlayerController>(GetOwningPlayer()))
			{
				const AActor* StashOwner = StashComponent->GetOwner();
				if (!StashOwner || !StashOwner->HasAuthority())
				{
					PlayerController->ServerStoreInventorySlotQuantityToStash(StashComponent, SourceSlotIndex, TargetSlotIndex, Quantity);
					return;
				}
			}
=======
>>>>>>> origin/main
			StashComponent->StoreInventorySlotQuantityToSlot(InventoryComponent, SourceSlotIndex, TargetSlotIndex, Quantity);
		}
		break;
	case EPDItemContainerType::Stash:
<<<<<<< HEAD
		if (APDPlayerController* PlayerController = Cast<APDPlayerController>(GetOwningPlayer()))
		{
			const AActor* StashOwner = StashComponent->GetOwner();
			if (!StashOwner || !StashOwner->HasAuthority())
			{
				PlayerController->ServerMoveStashSlotQuantity(StashComponent, SourceSlotIndex, TargetSlotIndex, Quantity);
				return;
			}
		}
		StashComponent->MoveSlotQuantityToSlot(SourceSlotIndex, TargetSlotIndex, Quantity);
		break;
	case EPDItemContainerType::QuickSlot:


=======
		StashComponent->MoveSlotQuantityToSlot(SourceSlotIndex, TargetSlotIndex, Quantity);
		break;
	case EPDItemContainerType::QuickSlot:
		if (UPDQuickSlotComponent* QuickSlotComponent = FindQuickSlotComponent())
		{
			QuickSlotComponent->TakeQuickSlotQuantityToStashSlot(StashComponent, SourceSlotIndex, TargetSlotIndex, Quantity);
		}
>>>>>>> origin/main
		break;
	default:
		break;
	}
}

bool UPDStashWidget::ShouldOpenTransferQuantityPopup(EPDItemContainerType SourceContainerType, int32 SourceSlotIndex, int32 TargetSlotIndex, int32& OutMaxQuantity, FText& OutTitle) const
{
	OutMaxQuantity = 0;
	OutTitle = FText::GetEmpty();

	if (SourceContainerType == EPDItemContainerType::Stash)
	{
		return false;
	}

	const FPDInventorySlot* SourceSlot = FindSourceSlot(SourceContainerType, SourceSlotIndex);
	const FPDInventorySlot* TargetSlot = FindStashSlot(TargetSlotIndex);
	if (!SourceSlot || !TargetSlot || SourceSlot->IsEmpty())
	{
		return false;
	}

	OutMaxQuantity = FPDItemSlotTransfer::GetQuantityLimit(*SourceSlot, *TargetSlot);
	OutTitle = SourceSlot->ItemData.DisplayName.IsEmpty() ? FText::FromName(SourceSlot->ItemData.ItemID) : SourceSlot->ItemData.DisplayName;
	return FPDItemSlotTransfer::CanPromptForPartial(*SourceSlot, *TargetSlot) && OutMaxQuantity > 1;
}

void UPDStashWidget::OpenQuantityPopup(int32 SlotIndex, int32 MaxQuantity, const FText& Title)
{
	if (!QuantityPopupWidgetClass)
	{
		TakeStashSlotQuantity(SlotIndex, MaxQuantity);
		return;
	}

	if (ActiveQuantityPopup && ActiveQuantityPopup->IsInViewport())
	{
		ActiveQuantityPopup->RemoveFromParent();
	}

	ActiveQuantityPopup = CreateWidget<UPDQuantityPopupWidget>(GetOwningPlayer(), QuantityPopupWidgetClass);
	if (!ActiveQuantityPopup)
	{
		TakeStashSlotQuantity(SlotIndex, MaxQuantity);
		return;
	}

	bPendingTransferQuantityRequest = false;
	PendingSlotIndex = SlotIndex;
	ActiveQuantityPopup->OnConfirmed.RemoveDynamic(this, &UPDStashWidget::HandleQuantityConfirmed);
	ActiveQuantityPopup->OnConfirmed.AddUniqueDynamic(this, &UPDStashWidget::HandleQuantityConfirmed);
	ActiveQuantityPopup->OnCancelled.RemoveDynamic(this, &UPDStashWidget::HandleQuantityCancelled);
	ActiveQuantityPopup->OnCancelled.AddUniqueDynamic(this, &UPDStashWidget::HandleQuantityCancelled);
<<<<<<< HEAD
=======
	ActiveQuantityPopup->AddToViewport(100);
	ActiveQuantityPopup->InitializeQuantityPopup(MaxQuantity, Title);
}

void UPDStashWidget::OpenTransferQuantityPopup(EPDItemContainerType SourceContainerType, int32 SourceSlotIndex, int32 TargetSlotIndex, int32 MaxQuantity, const FText& Title)
{
	if (!QuantityPopupWidgetClass)
	{
		ExecuteStashSlotTransfer(SourceContainerType, SourceSlotIndex, TargetSlotIndex, MaxQuantity);
		return;
	}

	if (ActiveQuantityPopup && ActiveQuantityPopup->IsInViewport())
	{
		ActiveQuantityPopup->RemoveFromParent();
	}

	ActiveQuantityPopup = CreateWidget<UPDQuantityPopupWidget>(GetOwningPlayer(), QuantityPopupWidgetClass);
	if (!ActiveQuantityPopup)
	{
		ExecuteStashSlotTransfer(SourceContainerType, SourceSlotIndex, TargetSlotIndex, MaxQuantity);
		return;
	}

	bPendingTransferQuantityRequest = true;
	PendingTransferSourceContainerType = SourceContainerType;
	PendingTransferSourceSlotIndex = SourceSlotIndex;
	PendingTransferTargetSlotIndex = TargetSlotIndex;
	PendingSlotIndex = INDEX_NONE;
	ActiveQuantityPopup->OnConfirmed.RemoveDynamic(this, &UPDStashWidget::HandleQuantityConfirmed);
	ActiveQuantityPopup->OnConfirmed.AddUniqueDynamic(this, &UPDStashWidget::HandleQuantityConfirmed);
	ActiveQuantityPopup->OnCancelled.RemoveDynamic(this, &UPDStashWidget::HandleQuantityCancelled);
	ActiveQuantityPopup->OnCancelled.AddUniqueDynamic(this, &UPDStashWidget::HandleQuantityCancelled);
>>>>>>> origin/main
	ActiveQuantityPopup->AddToViewport(100);
	if (const FPDInventorySlot* PreviewSlot = FindStashSlot(SlotIndex))
	{
		ActiveQuantityPopup->InitializeQuantityPopupWithSlot(MaxQuantity, Title, *PreviewSlot);
	}
	else
	{
		ActiveQuantityPopup->InitializeQuantityPopup(MaxQuantity, Title);
	}
}

void UPDStashWidget::OpenTransferQuantityPopup(EPDItemContainerType SourceContainerType, int32 SourceSlotIndex, int32 TargetSlotIndex, int32 MaxQuantity, const FText& Title)
{
	if (!QuantityPopupWidgetClass)
	{
		ExecuteStashSlotTransfer(SourceContainerType, SourceSlotIndex, TargetSlotIndex, MaxQuantity);
		return;
	}

	if (ActiveQuantityPopup && ActiveQuantityPopup->IsInViewport())
	{
		ActiveQuantityPopup->RemoveFromParent();
	}

	ActiveQuantityPopup = CreateWidget<UPDQuantityPopupWidget>(GetOwningPlayer(), QuantityPopupWidgetClass);
	if (!ActiveQuantityPopup)
	{
		ExecuteStashSlotTransfer(SourceContainerType, SourceSlotIndex, TargetSlotIndex, MaxQuantity);
		return;
	}

	bPendingTransferQuantityRequest = true;
	PendingTransferSourceContainerType = SourceContainerType;
	PendingTransferSourceSlotIndex = SourceSlotIndex;
	PendingTransferTargetSlotIndex = TargetSlotIndex;
	PendingSlotIndex = INDEX_NONE;
	ActiveQuantityPopup->OnConfirmed.RemoveDynamic(this, &UPDStashWidget::HandleQuantityConfirmed);
	ActiveQuantityPopup->OnConfirmed.AddUniqueDynamic(this, &UPDStashWidget::HandleQuantityConfirmed);
	ActiveQuantityPopup->OnCancelled.RemoveDynamic(this, &UPDStashWidget::HandleQuantityCancelled);
	ActiveQuantityPopup->OnCancelled.AddUniqueDynamic(this, &UPDStashWidget::HandleQuantityCancelled);
	ActiveQuantityPopup->AddToViewport(100);
	if (const FPDInventorySlot* PreviewSlot = FindSourceSlot(SourceContainerType, SourceSlotIndex))
	{
		ActiveQuantityPopup->InitializeQuantityPopupWithSlot(MaxQuantity, Title, *PreviewSlot);
	}
	else
	{
		ActiveQuantityPopup->InitializeQuantityPopup(MaxQuantity, Title);
	}
}

void UPDStashWidget::HandleQuantityConfirmed(int32 Quantity)
{
	if (bPendingTransferQuantityRequest)
	{
		ExecuteStashSlotTransfer(PendingTransferSourceContainerType, PendingTransferSourceSlotIndex, PendingTransferTargetSlotIndex, Quantity);
	}
	else if (PendingSlotIndex != INDEX_NONE)
	{
		TakeStashSlotQuantity(PendingSlotIndex, Quantity);
	}

	ClearQuantityRequest();
}

void UPDStashWidget::HandleQuantityCancelled()
{
	ClearQuantityRequest();
}

void UPDStashWidget::ClearQuantityRequest()
{
	bPendingTransferQuantityRequest = false;
	PendingTransferSourceContainerType = EPDItemContainerType::None;
	PendingTransferSourceSlotIndex = INDEX_NONE;
	PendingTransferTargetSlotIndex = INDEX_NONE;
	PendingSlotIndex = INDEX_NONE;
	ActiveQuantityPopup = nullptr;
}

void UPDStashWidget::BindStashChanged()
{
	UPDStashComponent* StashComponent = FindStashComponent();

	if (BoundStashComponent == StashComponent)
	{
		return;
	}

	UnbindStashChanged();

	BoundStashComponent = StashComponent;

	if (BoundStashComponent)
	{
		BoundStashComponent->OnStashChanged.AddUniqueDynamic(this, &UPDStashWidget::RefreshStashGrid);
	}
}

void UPDStashWidget::UnbindStashChanged()
{
	if (BoundStashComponent)
	{
		BoundStashComponent->OnStashChanged.RemoveDynamic(this, &UPDStashWidget::RefreshStashGrid);
		BoundStashComponent = nullptr;
	}
}
