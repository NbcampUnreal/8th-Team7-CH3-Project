
#include "Widgets/Inventory/PDInventoryWidget.h"
#include "Widgets/PDWidgetSoundLibrary.h"

#include "Algo/Sort.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/ScaleBox.h"
#include "Components/ScaleBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/SizeBoxSlot.h"
#include "Components/TextBlock.h"
<<<<<<< HEAD
#include "Components/Widget.h"
#include "Components/PanelWidget.h"
#include "Components/Button.h"
#include "Core/PDPlayerComponentResolver.h"
=======
#include "Components/PanelWidget.h"
>>>>>>> origin/main
#include "Core/PDPlayerController.h"
#include "Items/Containers/PDLootComponent.h"
#include "GameFramework/Pawn.h"
<<<<<<< HEAD
#include "Items/Containers/PDInventoryComponent.h"
#include "Items/Data/PDItemSlotTransfer.h"
#include "Items/Containers/PDQuickSlotComponent.h"
#include "Items/Containers/PDSecureContainerComponent.h"
#include "Characters/PDPlayerCharacter.h"
#include "Items/Equipment/PDEquipmentComponent.h"
#include "Items/Containers/PDStashComponent.h"
=======
#include "Items/PDInventoryComponent.h"
#include "Items/PDItemSlotTransfer.h"
#include "Items/PDQuickSlotComponent.h"
#include "Items/PDStashComponent.h"
>>>>>>> origin/main
#include "Widgets/Inventory/PDInventoryItemContextMenuWidget.h"
#include "Widgets/Inventory/PDInventorySlotWidget.h"
#include "Widgets/Inventory/PDInventoryWeightBarWidget.h"
#include "Widgets/Inventory/PDQuantityPopupWidget.h"

void UPDInventoryWidget::InitializeForOwner(APlayerController* /*OwnerPC*/)
{
}

void UPDInventoryWidget::OnTabShown()
{
}

void UPDInventoryWidget::OnTabHidden()
{
	CloseContextMenu();
	CloseItemHoverTooltip();
}

void UPDInventoryWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
}

void UPDInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BindInventoryChanged();
	BindEquipmentChanged();
	BindTabButtons();
	BindSortButtons();
	CacheFilterTabBaseLabels();
	SetSortOptionsVisible(false);
	ResolveEquipmentSlotWidgets();
	RefreshEquipmentSlots();
	RefreshInventoryGrid();
	UpdateTabButtonStyle();
}

void UPDInventoryWidget::NativeDestruct()
{
	CloseContextMenu();
	CloseItemHoverTooltip();
	if (ActiveQuantityPopup && ActiveQuantityPopup->IsInViewport())
	{
		ActiveQuantityPopup->RemoveFromParent();
	}
	ClearQuantityRequest();
<<<<<<< HEAD
	UnbindEquipmentChanged();
=======
>>>>>>> origin/main
	UnbindInventoryChanged();
	Super::NativeDestruct();
}

void UPDInventoryWidget::RefreshInventoryGrid()
{
	CloseContextMenu();
	CloseItemHoverTooltip();
	ResolveInventoryGridPanel();

	if (!InventoryGridPanel)
	{
		UE_LOG(LogTemp, Warning, TEXT("PDInventoryWidget: Inventory grid widget was not found. Expected widget name: %s"), *InventoryGridWidgetName.ToString());
		return;
	}

	InventoryGridPanel->ClearChildren();
	RefreshGoldText();
	RefreshInventoryWeightBar();

	if (!InventorySlotWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("PDInventoryWidget: InventorySlotWidgetClass is not set. Set it to WBP_InventorySlot in WBP_Inventory Class Defaults."));
		return;
	}

	UPDInventoryComponent* InventoryComponent = FindInventoryComponent();

	if (!InventoryComponent)
	{
		return;
	}

	const int32 Columns = FMath::Max(1, InventoryComponent->GridColumns);
	const int32 Rows = FMath::Max(1, InventoryComponent->GridRows);
	const int32 SlotCount = Columns * Rows;
	const float SlotWidth = FMath::Max(1.f, InventorySlotWidth);
	const float SlotHeight = FMath::Max(1.f, InventorySlotHeight);

	InventoryGridPanel->SetMinDesiredSlotWidth(SlotWidth);
	InventoryGridPanel->SetMinDesiredSlotHeight(SlotHeight);

	TArray<int32> DisplaySlotIndices;
	if (InventoryComponent)
	{
		for (int32 RealSlotIndex = 0; RealSlotIndex < InventoryComponent->Items.Num(); ++RealSlotIndex)
		{
			const FPDInventorySlot& InventorySlotData = InventoryComponent->Items[RealSlotIndex];
			if (!InventorySlotData.IsEmpty() && DoesSlotMatchCurrentFilter(InventorySlotData))
			{
				DisplaySlotIndices.Add(RealSlotIndex);
			}
		}

		SortDisplaySlotIndices(DisplaySlotIndices, InventoryComponent);

		for (int32 RealSlotIndex = 0; RealSlotIndex < InventoryComponent->Items.Num(); ++RealSlotIndex)
		{
			const FPDInventorySlot& InventorySlotData = InventoryComponent->Items[RealSlotIndex];
			if (InventorySlotData.IsEmpty())
			{
				DisplaySlotIndices.Add(RealSlotIndex);
			}
		}
	}

	for (int32 DisplayIndex = 0; DisplayIndex < SlotCount; ++DisplayIndex)
	{
		UUserWidget* CreatedSlotWidget = CreateWidget<UUserWidget>(GetOwningPlayer(), InventorySlotWidgetClass);
		if (!CreatedSlotWidget)
		{
			continue;
		}

		if (UPDInventorySlotWidget* InventorySlotWidget = Cast<UPDInventorySlotWidget>(CreatedSlotWidget))
		{
			InventorySlotWidget->SetSlotContainerType(EPDItemContainerType::Inventory);
			InventorySlotWidget->OnSlotLeftClicked.AddUniqueDynamic(this, &UPDInventoryWidget::HandleInventorySlotLeftClicked);
			InventorySlotWidget->OnSlotRightClicked.AddUniqueDynamic(this, &UPDInventoryWidget::HandleInventorySlotRightClicked);
			InventorySlotWidget->OnSlotHovered.AddUniqueDynamic(this, &UPDInventoryWidget::HandleInventorySlotHovered);
			InventorySlotWidget->OnSlotUnhovered.AddUniqueDynamic(this, &UPDInventoryWidget::HandleInventorySlotUnhovered);
			InventorySlotWidget->OnSlotItemDropped.AddUniqueDynamic(this, &UPDInventoryWidget::HandleInventorySlotItemDropped);

			if (InventoryComponent && DisplaySlotIndices.IsValidIndex(DisplayIndex))
			{
				const int32 RealSlotIndex = DisplaySlotIndices[DisplayIndex];
				if (InventoryComponent->Items.IsValidIndex(RealSlotIndex) && !InventoryComponent->Items[RealSlotIndex].IsEmpty())
				{
					InventorySlotWidget->SetSlotData(InventoryComponent->Items[RealSlotIndex], RealSlotIndex);
				}
				else
				{
					InventorySlotWidget->ClearSlotData(RealSlotIndex);
				}
			}
			else
			{
				InventorySlotWidget->ClearSlotData(INDEX_NONE);
			}
		}

		UWidget* GridChildWidget = CreatedSlotWidget;
		USizeBox* SlotSizeBox = WidgetTree ? WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass()) : nullptr;
		if (SlotSizeBox)
		{
			SlotSizeBox->SetWidthOverride(SlotWidth);
			SlotSizeBox->SetHeightOverride(SlotHeight);
			SlotSizeBox->SetMinDesiredWidth(SlotWidth);
			SlotSizeBox->SetMinDesiredHeight(SlotHeight);

			UWidget* SlotContentWidget = CreatedSlotWidget;
			if (bScaleInventorySlotWidgetToFit)
			{
				UScaleBox* SlotScaleBox = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass());
				if (SlotScaleBox)
				{
					SlotScaleBox->SetStretch(EStretch::ScaleToFit);
					SlotScaleBox->SetStretchDirection(EStretchDirection::Both);

					if (UScaleBoxSlot* ScaleBoxSlot = Cast<UScaleBoxSlot>(SlotScaleBox->AddChild(CreatedSlotWidget)))
					{
						ScaleBoxSlot->SetHorizontalAlignment(HAlign_Fill);
						ScaleBoxSlot->SetVerticalAlignment(VAlign_Fill);
					}

					SlotContentWidget = SlotScaleBox;
				}
			}

			if (USizeBoxSlot* SizeBoxSlot = Cast<USizeBoxSlot>(SlotSizeBox->AddChild(SlotContentWidget)))
			{
				SizeBoxSlot->SetHorizontalAlignment(HAlign_Fill);
				SizeBoxSlot->SetVerticalAlignment(VAlign_Fill);
			}

			GridChildWidget = SlotSizeBox;
		}

		UUniformGridSlot* GridSlot = InventoryGridPanel->AddChildToUniformGrid(GridChildWidget, DisplayIndex / Columns, DisplayIndex % Columns);
		if (GridSlot)
		{
			GridSlot->SetHorizontalAlignment(HAlign_Center);
			GridSlot->SetVerticalAlignment(VAlign_Center);
		}
	}

	RefreshEquipmentSlots();
	UpdateTabButtonStyle();
}

void UPDInventoryWidget::SetInventoryFilterTab(EPDItemFilterTab NewFilterTab)
{
	if (CurrentFilterTab == NewFilterTab)
	{
		UpdateTabButtonStyle();
		return;
	}

	CurrentFilterTab = NewFilterTab;
	CurrentSortMode = EPDItemSortMode::None;
	SetSortOptionsVisible(false);
	RefreshInventoryGrid();
}

void UPDInventoryWidget::SetInventorySortMode(EPDItemSortMode NewSortMode)
{
	if (CurrentSortMode == NewSortMode)
	{
		SetSortOptionsVisible(false);
		return;
	}

	CurrentSortMode = NewSortMode;
	SetSortOptionsVisible(false);
	RefreshInventoryGrid();
}

void UPDInventoryWidget::BindTabButtons()
{
	if (Button_Equipment)
	{
		Button_Equipment->OnClicked.AddUniqueDynamic(this, &UPDInventoryWidget::HandleEquipmentTabClicked);
	}

	if (Button_Consumable)
	{
		Button_Consumable->OnClicked.AddUniqueDynamic(this, &UPDInventoryWidget::HandleConsumableTabClicked);
	}

	if (Button_Misc)
	{
		Button_Misc->OnClicked.AddUniqueDynamic(this, &UPDInventoryWidget::HandleMiscTabClicked);
	}
}

void UPDInventoryWidget::BindSortButtons()
{
	if (Button_Sort)
	{
		Button_Sort->OnClicked.AddUniqueDynamic(this, &UPDInventoryWidget::HandleSortButtonClicked);
	}

	if (Button_SortByName)
	{
		Button_SortByName->OnClicked.AddUniqueDynamic(this, &UPDInventoryWidget::HandleSortByNameClicked);
	}

	if (Button_SortByType)
	{
		Button_SortByType->OnClicked.AddUniqueDynamic(this, &UPDInventoryWidget::HandleSortByTypeClicked);
	}

	if (Button_SortTab_Name)
	{
		Button_SortTab_Name->OnClicked.AddUniqueDynamic(this, &UPDInventoryWidget::HandleSortByNameClicked);
	}

	if (Button_SortTab_Type)
	{
		Button_SortTab_Type->OnClicked.AddUniqueDynamic(this, &UPDInventoryWidget::HandleSortByTypeClicked);
	}
}

void UPDInventoryWidget::UpdateTabButtonStyle()
{
	if (FilterTabBaseLabels.IsEmpty())
	{
		CacheFilterTabBaseLabels();
	}

	const FLinearColor SelectedColor(0.15f, 0.85f, 0.15f, 1.0f);
	const FLinearColor NormalColor(0.02f, 0.02f, 0.02f, 0.85f);

	const int32 MaxSlots = GetInventoryDisplaySlotCount();
	const int32 EquipmentUsedSlots = CountOccupiedInventorySlotsByType(EPDItemType::Equipment);
	const int32 ConsumableUsedSlots = CountOccupiedInventorySlotsByType(EPDItemType::Consumable);
	const int32 MiscUsedSlots = CountOccupiedInventorySlotsByType(EPDItemType::Misc);

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

int32 UPDInventoryWidget::CountOccupiedInventorySlotsByType(EPDItemType ItemType) const
{
	const UPDInventoryComponent* InventoryComponent = FindInventoryComponent();
	if (!InventoryComponent)
	{
		return 0;
	}

	int32 UsedSlotCount = 0;
	for (const FPDInventorySlot& InventorySlotData : InventoryComponent->Items)
	{
		if (!InventorySlotData.IsEmpty() && InventorySlotData.ItemData.ItemType == ItemType)
		{
			++UsedSlotCount;
		}
	}

	return UsedSlotCount;
}

int32 UPDInventoryWidget::GetInventoryDisplaySlotCount() const
{
	const UPDInventoryComponent* InventoryComponent = FindInventoryComponent();
	if (InventoryComponent)
	{
		return FMath::Max(1, InventoryComponent->GetMaxSlotCount());
	}

	return 16;
}


void UPDInventoryWidget::CacheFilterTabBaseLabels()
{
	FilterTabBaseLabels.Reset();
	CacheFilterTabBaseLabel(EPDItemFilterTab::Equipment, Button_Equipment);
	CacheFilterTabBaseLabel(EPDItemFilterTab::Consumable, Button_Consumable);
	CacheFilterTabBaseLabel(EPDItemFilterTab::Misc, Button_Misc);
}

void UPDInventoryWidget::CacheFilterTabBaseLabel(EPDItemFilterTab FilterTab, UButton* TargetButton)
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

FText UPDInventoryWidget::GetFilterTabBaseLabel(EPDItemFilterTab FilterTab) const
{
	if (const FText* BaseLabel = FilterTabBaseLabels.Find(FilterTab))
	{
		return *BaseLabel;
	}

	return FText::GetEmpty();
}

UTextBlock* UPDInventoryWidget::GetTabButtonTextBlock(UButton* TargetButton) const
{
	return TargetButton ? Cast<UTextBlock>(TargetButton->GetContent()) : nullptr;
}

void UPDInventoryWidget::SetTabButtonLabel(UButton* TargetButton, const FText& BaseLabel, int32 UsedSlots, int32 MaxSlots) const
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

bool UPDInventoryWidget::DoesSlotMatchCurrentFilter(const FPDInventorySlot& InventorySlotData) const
{
	return !InventorySlotData.IsEmpty() && DoesItemTypeMatchCurrentFilter(InventorySlotData.ItemData.ItemType);
}

bool UPDInventoryWidget::DoesItemTypeMatchCurrentFilter(EPDItemType ItemType) const
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

bool UPDInventoryWidget::CanAcceptDropForCurrentFilter(const UPDInventoryDragDropOperation* DragOperation) const
{
	return DragOperation
		&& DragOperation->IsValidPayload()
		&& !DragOperation->SlotData.IsEmpty()
		&& DoesItemTypeMatchCurrentFilter(DragOperation->SlotData.ItemData.ItemType);
}

void UPDInventoryWidget::SortDisplaySlotIndices(TArray<int32>& DisplaySlotIndices, const UPDInventoryComponent* InventoryComponent) const
{
	if (!InventoryComponent || CurrentSortMode == EPDItemSortMode::None)
	{
		return;
	}

	Algo::Sort(DisplaySlotIndices, [this, InventoryComponent](int32 LeftIndex, int32 RightIndex)
	{
		const FPDInventorySlot& LeftSlot = InventoryComponent->Items[LeftIndex];
		const FPDInventorySlot& RightSlot = InventoryComponent->Items[RightIndex];

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

void UPDInventoryWidget::SetSortOptionsVisible(bool bVisible)
{
	UWidget* SortPanel = Panel_SortTabs ? Panel_SortTabs.Get() : Panel_SortOptions.Get();
	if (SortPanel)
	{
		SortPanel->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UPDInventoryWidget::ToggleSortOptions()
{
	UWidget* SortPanel = Panel_SortTabs ? Panel_SortTabs.Get() : Panel_SortOptions.Get();
	if (SortPanel)
	{
		SetSortOptionsVisible(SortPanel->GetVisibility() != ESlateVisibility::Visible);
	}
}

void UPDInventoryWidget::HandleEquipmentTabClicked()
{
	UPDWidgetSoundLibrary::PlayUISound2D(this, ButtonClickSound);

	SetInventoryFilterTab(EPDItemFilterTab::Equipment);
}

void UPDInventoryWidget::HandleConsumableTabClicked()
{
	UPDWidgetSoundLibrary::PlayUISound2D(this, ButtonClickSound);

	SetInventoryFilterTab(EPDItemFilterTab::Consumable);
}

void UPDInventoryWidget::HandleMiscTabClicked()
{
	UPDWidgetSoundLibrary::PlayUISound2D(this, ButtonClickSound);

	SetInventoryFilterTab(EPDItemFilterTab::Misc);
}

void UPDInventoryWidget::HandleSortButtonClicked()
{
	UPDWidgetSoundLibrary::PlayUISound2D(this, ButtonClickSound);

	ToggleSortOptions();
}

void UPDInventoryWidget::HandleSortByNameClicked()
{
	UPDWidgetSoundLibrary::PlayUISound2D(this, ButtonClickSound);

	SetInventorySortMode(EPDItemSortMode::Name);
}

void UPDInventoryWidget::HandleSortByTypeClicked()
{
	UPDWidgetSoundLibrary::PlayUISound2D(this, ButtonClickSound);

	SetInventorySortMode(EPDItemSortMode::Type);
}


void UPDInventoryWidget::ResolveEquipmentSlotWidgets()
{
	EquipmentSlotWidgets.Reset();
	RegisterEquipmentSlotWidget(EPDEquipmentSlotType::Weapon, EquipmentSlotWeaponWidgetName);
	RegisterEquipmentSlotWidget(EPDEquipmentSlotType::Head, EquipmentSlotHeadWidgetName);
	RegisterEquipmentSlotWidget(EPDEquipmentSlotType::Armor, EquipmentSlotArmorWidgetName);
	RegisterEquipmentSlotWidget(EPDEquipmentSlotType::Bag, EquipmentSlotBagWidgetName);
}

void UPDInventoryWidget::RegisterEquipmentSlotWidget(EPDEquipmentSlotType SlotType, FName WidgetName)
{
	if (!WidgetTree || WidgetName.IsNone())
	{
		return;
	}

	UPDInventorySlotWidget* EquipmentSlotWidget = Cast<UPDInventorySlotWidget>(WidgetTree->FindWidget(WidgetName));
	if (!EquipmentSlotWidget)
	{
		return;
	}

	const int32 SlotIndex = static_cast<int32>(SlotType);
	EquipmentSlotWidget->SetSlotContainerType(EPDItemContainerType::Equipment);
	EquipmentSlotWidget->SetEmptySlotLabel(GetEquipmentSlotLabel(SlotType));
	EquipmentSlotWidget->ClearSlotData(SlotIndex);
	EquipmentSlotWidget->OnSlotRightClicked.AddUniqueDynamic(this, &UPDInventoryWidget::HandleEquipmentSlotRightClicked);
	EquipmentSlotWidget->OnSlotItemDropped.AddUniqueDynamic(this, &UPDInventoryWidget::HandleEquipmentSlotItemDropped);
	EquipmentSlotWidgets.Add(SlotType, EquipmentSlotWidget);
}


FText UPDInventoryWidget::GetEquipmentSlotLabel(EPDEquipmentSlotType SlotType) const
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

void UPDInventoryWidget::BindEquipmentChanged()
{
	UPDEquipmentComponent* EquipmentComponent = FindEquipmentComponent();
	if (BoundEquipmentComponent == EquipmentComponent)
	{
		return;
	}

	UnbindEquipmentChanged();
	BoundEquipmentComponent = EquipmentComponent;
	if (BoundEquipmentComponent)
	{
		BoundEquipmentComponent->OnEquipmentChanged.AddUniqueDynamic(this, &UPDInventoryWidget::HandleEquipmentChanged);
	}
}

void UPDInventoryWidget::UnbindEquipmentChanged()
{
	if (BoundEquipmentComponent)
	{
		BoundEquipmentComponent->OnEquipmentChanged.RemoveDynamic(this, &UPDInventoryWidget::HandleEquipmentChanged);
		BoundEquipmentComponent = nullptr;
	}
}

void UPDInventoryWidget::RefreshEquipmentSlots()
{
	UPDEquipmentComponent* EquipmentComponent = FindEquipmentComponent();
	for (const TPair<EPDEquipmentSlotType, TWeakObjectPtr<UPDInventorySlotWidget>>& Pair : EquipmentSlotWidgets)
	{
		UPDInventorySlotWidget* EquipmentSlotWidget = Pair.Value.Get();
		if (!EquipmentSlotWidget)
		{
			continue;
		}

		const int32 SlotIndex = static_cast<int32>(Pair.Key);
		EquipmentSlotWidget->SetSlotContainerType(EPDItemContainerType::Equipment);
		EquipmentSlotWidget->SetEmptySlotLabel(GetEquipmentSlotLabel(Pair.Key));

		if (EquipmentComponent)
		{
			const FPDInventorySlot EquippedSlot = EquipmentComponent->GetEquippedSlot(Pair.Key);
			if (!EquippedSlot.IsEmpty())
			{
				EquipmentSlotWidget->SetSlotData(EquippedSlot, SlotIndex);
				continue;
			}
		}

		EquipmentSlotWidget->ClearSlotData(SlotIndex);
	}
}

void UPDInventoryWidget::HandleEquipmentSlotRightClicked(UPDInventorySlotWidget* SlotWidget, int32 EquipmentSlotIndex)
{
	UPDInventoryComponent* InventoryComponent = FindInventoryComponent();
	UPDEquipmentComponent* EquipmentComponent = FindEquipmentComponent();
	const EPDEquipmentSlotType SlotType = static_cast<EPDEquipmentSlotType>(EquipmentSlotIndex);
	if (!SlotWidget || !InventoryComponent || !EquipmentComponent || SlotType == EPDEquipmentSlotType::None)
	{
		return;
	}

	if (EquipmentComponent->UnequipItemToInventory(InventoryComponent, SlotType))
	{
		RefreshEquipmentSlots();
		RefreshInventoryGrid();
	}
}


void UPDInventoryWidget::HandleEquipmentSlotItemDropped(UPDInventorySlotWidget* SlotWidget, int32 EquipmentSlotIndex, UPDInventoryDragDropOperation* DragOperation)
{
	UPDInventoryComponent* InventoryComponent = FindInventoryComponent();
	UPDEquipmentComponent* EquipmentComponent = FindEquipmentComponent();
	const EPDEquipmentSlotType SlotType = static_cast<EPDEquipmentSlotType>(EquipmentSlotIndex);

	if (!SlotWidget || !DragOperation || !InventoryComponent || !EquipmentComponent || SlotType == EPDEquipmentSlotType::None)
	{
		return;
	}

	if (DragOperation->SourceContainerType == EPDItemContainerType::Equipment)
	{
		const EPDEquipmentSlotType SourceSlotType = static_cast<EPDEquipmentSlotType>(DragOperation->SourceSlotIndex);
		if (SourceSlotType != SlotType)
		{
			return;
		}

		RefreshEquipmentSlots();
		return;
	}

	if (DragOperation->SourceContainerType != EPDItemContainerType::Inventory)
	{
		return;
	}

	const FPDInventorySlot* SourceSlot = FindInventorySlot(DragOperation->SourceSlotIndex);
	if (!SourceSlot || SourceSlot->IsEmpty())
	{
		return;
	}

	if (EquipmentComponent->ResolveEquipmentSlotType(SourceSlot->ItemData) != SlotType)
	{
		return;
	}

	if (SlotType == EPDEquipmentSlotType::Weapon)
	{
		if (UPDQuickSlotComponent* QuickSlotComponent = FindQuickSlotComponent())
		{
			if (QuickSlotComponent->EquipInventoryWeaponSlot(DragOperation->SourceSlotIndex))
			{
				RefreshEquipmentSlots();
				RefreshInventoryGrid();
			}
		}
		return;
	}

	if (EquipmentComponent->EquipItemFromInventoryToSlot(InventoryComponent, DragOperation->SourceSlotIndex, SlotType))
	{
		RefreshEquipmentSlots();
		RefreshInventoryGrid();
	}
}


void UPDInventoryWidget::ResolveInventoryGridPanel()
{
	if (InventoryGridPanel)
	{
		return;
	}

	if (!WidgetTree)
	{
		return;
	}

	if (UWidget* FoundWidget = WidgetTree->FindWidget(InventoryGridWidgetName))
	{
		InventoryGridPanel = Cast<UUniformGridPanel>(FoundWidget);
	}

	if (!InventoryGridPanel)
	{
		WidgetTree->ForEachWidget([this](UWidget* Widget)
		{
			if (!InventoryGridPanel)
			{
				InventoryGridPanel = Cast<UUniformGridPanel>(Widget);
			}
		});
	}
}


void UPDInventoryWidget::RefreshGoldText()
{
	if (!GoldTextWidget)
	{
		GoldTextWidget = Text_Gold;
	}

	if (!GoldTextWidget && WidgetTree && !GoldTextWidgetName.IsNone())
	{
		GoldTextWidget = Cast<UTextBlock>(WidgetTree->FindWidget(GoldTextWidgetName));
	}

	if (!GoldTextWidget)
	{
		return;
	}

	const UPDInventoryComponent* InventoryComponent = FindInventoryComponent();
	GoldTextWidget->SetText(FText::AsNumber(FMath::Max(0, InventoryComponent ? InventoryComponent->GetGold() : 0)));
}

void UPDInventoryWidget::ResolveInventoryWeightBarWidget()
{
	if (InventoryWeightBarWidget)
	{
		return;
	}

	if (!WidgetTree)
	{
		return;
	}

	if (!InventoryWeightBarWidgetName.IsNone())
	{
		InventoryWeightBarWidget = Cast<UPDInventoryWeightBarWidget>(WidgetTree->FindWidget(InventoryWeightBarWidgetName));
	}

	if (InventoryWeightBarWidget)
	{
		return;
	}

	WidgetTree->ForEachWidget([this](UWidget* Widget)
	{
		if (!InventoryWeightBarWidget)
		{
			InventoryWeightBarWidget = Cast<UPDInventoryWeightBarWidget>(Widget);
		}
	});
}

void UPDInventoryWidget::RefreshInventoryWeightBar()
{
	ResolveInventoryWeightBarWidget();

	if (!InventoryWeightBarWidget)
	{
		return;
	}

	const UPDInventoryComponent* InventoryComponent = FindInventoryComponent();
	const float CurrentWeight = InventoryComponent ? InventoryComponent->GetCurrentWeight() : 0.f;
	const float MaxWeight = InventoryComponent ? InventoryComponent->GetMaxWeight() : 0.f;
	InventoryWeightBarWidget->SetWeight(CurrentWeight, MaxWeight);
}

UPDInventoryComponent* UPDInventoryWidget::FindInventoryComponent() const
{
	// 2�?구조: InventoryComponent??PlayerState???�으므�?PlayerController ?�퍼 ?�용.
	if (UPDInventoryComponent* Inventory = FPDPlayerComponentResolver::ResolveInventory(GetOwningPlayer()))
	{
		return Inventory;
	}
	// Fallback: Pawn??직접 붙�? 경우???�래 구조 ?�환.
	return FPDPlayerComponentResolver::ResolveInventory(GetOwningPlayerPawn());
}

UPDStashComponent* UPDInventoryWidget::FindStashComponent() const
{
	return ActiveStashComponent;
}

void UPDInventoryWidget::SetActiveStashComponent(UPDStashComponent* InStashComponent)
{
	ActiveStashComponent = InStashComponent;
}

void UPDInventoryWidget::SetLootCompanionMode(bool bEnabled)
{
	if (bEnabled)
	{
		SetRenderTranslation(LootCompanionTranslation);
		SetRenderScale(FVector2D(LootCompanionScale, LootCompanionScale));
	}
	else
	{
		SetRenderTranslation(FVector2D::ZeroVector);
		SetRenderScale(FVector2D(1.f, 1.f));
	}
}

UPDQuickSlotComponent* UPDInventoryWidget::FindQuickSlotComponent() const
{
	if (UPDQuickSlotComponent* QuickSlot = FPDPlayerComponentResolver::ResolveQuickSlot(GetOwningPlayer()))
	{
		return QuickSlot;
	}
	return FPDPlayerComponentResolver::ResolveQuickSlot(GetOwningPlayerPawn());
}

UPDSecureContainerComponent* UPDInventoryWidget::FindSecureContainerComponent() const
{
	// SecureContainerComponent??PlayerCharacter??붙어?�음.
	if (APawn* OwningPawn = GetOwningPlayerPawn())
	{
		return OwningPawn->FindComponentByClass<UPDSecureContainerComponent>();
	}

	return nullptr;
}

UPDEquipmentComponent* UPDInventoryWidget::FindEquipmentComponent() const
{
	if (UPDEquipmentComponent* Equipment = FPDPlayerComponentResolver::ResolveEquipment(GetOwningPlayer()))
	{
		return Equipment;
	}
	return FPDPlayerComponentResolver::ResolveEquipment(GetOwningPlayerPawn());
}

void UPDInventoryWidget::HandleEquipmentChanged()
{
	RefreshEquipmentSlots();
	RefreshInventoryGrid();
}

void UPDInventoryWidget::HandleInventoryWeightLimitExceeded(float CurrentWeight, float MaxWeight)
{
	BP_OnInventoryWeightLimitExceeded(CurrentWeight, MaxWeight);
}

void UPDInventoryWidget::HandleInventoryMessage(const FText&)
{
}

const FPDInventorySlot* UPDInventoryWidget::FindInventorySlot(int32 SlotIndex) const
{
	const UPDInventoryComponent* InventoryComponent = FindInventoryComponent();
	return InventoryComponent && InventoryComponent->Items.IsValidIndex(SlotIndex) ? &InventoryComponent->Items[SlotIndex] : nullptr;
}

const FPDInventorySlot* UPDInventoryWidget::FindSourceSlot(EPDItemContainerType SourceContainerType, int32 SlotIndex) const
{
	switch (SourceContainerType)
	{
	case EPDItemContainerType::Inventory:
		return FindInventorySlot(SlotIndex);
	case EPDItemContainerType::Stash:
		if (const UPDStashComponent* StashComponent = FindStashComponent())
		{
			return StashComponent->StashItems.IsValidIndex(SlotIndex) ? &StashComponent->StashItems[SlotIndex] : nullptr;
		}
		return nullptr;
	case EPDItemContainerType::QuickSlot:
		if (const UPDQuickSlotComponent* QuickSlotComponent = FindQuickSlotComponent())
		{
			return QuickSlotComponent->GetResolvedQuickSlot(SlotIndex);
		}
		return nullptr;
	case EPDItemContainerType::Equipment:
		return nullptr;
	case EPDItemContainerType::SecureContainer:
		if (const UPDSecureContainerComponent* SecureContainerComponent = FindSecureContainerComponent())
		{
			return SecureContainerComponent->GetSecureSlot(SlotIndex);
		}
		return nullptr;
	case EPDItemContainerType::Loot:
		if (APDPlayerController* PlayerController = Cast<APDPlayerController>(GetOwningPlayer()))
		{
			if (const UPDLootComponent* LootComponent = PlayerController->GetActiveLootComponent())
			{
				return LootComponent->LootItems.IsValidIndex(SlotIndex) ? &LootComponent->LootItems[SlotIndex] : nullptr;
			}
		}
		return nullptr;
	default:
		return nullptr;
	}
}

UPDQuickSlotComponent* UPDInventoryWidget::FindQuickSlotComponent() const
{
	if (APawn* OwningPawn = GetOwningPlayerPawn())
	{
		return OwningPawn->FindComponentByClass<UPDQuickSlotComponent>();
	}

	return nullptr;
}

const FPDInventorySlot* UPDInventoryWidget::FindInventorySlot(int32 SlotIndex) const
{
	const UPDInventoryComponent* InventoryComponent = FindInventoryComponent();
	return InventoryComponent && InventoryComponent->Items.IsValidIndex(SlotIndex) ? &InventoryComponent->Items[SlotIndex] : nullptr;
}

const FPDInventorySlot* UPDInventoryWidget::FindSourceSlot(EPDItemContainerType SourceContainerType, int32 SlotIndex) const
{
	switch (SourceContainerType)
	{
	case EPDItemContainerType::Inventory:
		return FindInventorySlot(SlotIndex);
	case EPDItemContainerType::Stash:
		if (const UPDStashComponent* StashComponent = FindStashComponent())
		{
			return StashComponent->StashItems.IsValidIndex(SlotIndex) ? &StashComponent->StashItems[SlotIndex] : nullptr;
		}
		return nullptr;
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

void UPDInventoryWidget::HandleInventorySlotLeftClicked(UPDInventorySlotWidget* SlotWidget, int32 ClickedSlotIndex)
{
	if (!SlotWidget || !SlotWidget->WasLastClickWithControl() || SlotWidget->GetSlotData().IsEmpty())
	{
		return;
	}

	APDPlayerController* PlayerController = Cast<APDPlayerController>(GetOwningPlayer());
	if (!PlayerController || (!PlayerController->IsStashInterfaceOpen() && !PlayerController->IsMarketInterfaceOpen()))
	{
		return;
	}

	const FPDInventorySlot& SlotData = SlotWidget->GetSlotData();
	if (SlotData.Quantity <= 1 || FMath::Max(1, SlotData.ItemData.MaxStack) <= 1)
	{
		ExecuteInventoryQuickAction(ClickedSlotIndex, 1);
		return;
	}

	const FText DisplayName = SlotData.ItemData.DisplayName.IsEmpty() ? FText::FromName(SlotData.ItemData.ItemID) : SlotData.ItemData.DisplayName;
	OpenQuantityPopup(ClickedSlotIndex, SlotData.Quantity, DisplayName);
}

void UPDInventoryWidget::HandleInventorySlotItemDropped(UPDInventorySlotWidget* SlotWidget, int32 TargetSlotIndex, UPDInventoryDragDropOperation* DragOperation)
{
<<<<<<< HEAD
	if (!SlotWidget || !CanAcceptDropForCurrentFilter(DragOperation) || TargetSlotIndex == INDEX_NONE)
=======
	if (!SlotWidget || !DragOperation || !DragOperation->IsValidPayload())
>>>>>>> origin/main
	{
		return;
	}

	CloseContextMenu();
	CloseItemHoverTooltip();

	if (DragOperation->SourceContainerType == EPDItemContainerType::Inventory)
	{
		ExecuteInventorySlotTransfer(DragOperation->SourceContainerType, DragOperation->SourceSlotIndex, TargetSlotIndex, DragOperation->SlotData.Quantity);
		return;
	}

	int32 MaxQuantity = 0;
	FText Title;
	if (ShouldOpenTransferQuantityPopup(DragOperation->SourceContainerType, DragOperation->SourceSlotIndex, TargetSlotIndex, MaxQuantity, Title))
	{
		OpenTransferQuantityPopup(DragOperation->SourceContainerType, DragOperation->SourceSlotIndex, TargetSlotIndex, MaxQuantity, Title);
		return;
	}

	ExecuteInventorySlotTransfer(DragOperation->SourceContainerType, DragOperation->SourceSlotIndex, TargetSlotIndex, DragOperation->SlotData.Quantity);
}

void UPDInventoryWidget::HandleInventorySlotHovered(UPDInventorySlotWidget* SlotWidget, int32 HoveredSlotIndex)
{
	if (!SlotWidget || SlotWidget->GetSlotData().IsEmpty())
	{
		return;
	}

	if (ActiveContextMenu)
	{
		CloseContextMenu();
	}

	OpenItemHoverTooltip(SlotWidget);
}

void UPDInventoryWidget::HandleInventorySlotUnhovered(UPDInventorySlotWidget* SlotWidget, int32 UnhoveredSlotIndex)
{
	if (ActiveContextMenu)
	{
		return;
	}

	if (!SlotWidget || SlotWidget == ActiveTooltipSlot)
	{
		CloseItemHoverTooltip();
	}
}


void UPDInventoryWidget::HandleInventorySlotRightClicked(UPDInventorySlotWidget* SlotWidget, int32 ClickedSlotIndex)
{
	if (!bEnableContextMenu)
	{
		return;
	}

	if (!SlotWidget || SlotWidget->GetSlotData().IsEmpty())
	{
		CloseContextMenu();
		return;
	}

	OpenContextMenu(SlotWidget, ClickedSlotIndex);
}

void UPDInventoryWidget::OpenContextMenu(UPDInventorySlotWidget* SlotWidget, int32 SlotIndex)
{
	CloseContextMenu();
<<<<<<< HEAD
	CloseItemHoverTooltip();
=======
>>>>>>> origin/main

	if (!SlotWidget || !ContextMenuWidgetClass)
	{
		return;
	}

	const FPDInventorySlot& SlotData = SlotWidget->GetSlotData();
	if (SlotData.IsEmpty())
	{
		return;
	}

<<<<<<< HEAD
=======
	OpenItemHoverTooltip(SlotWidget);

	UPanelWidget* ContextMenuContainer = FindContextMenuContainer();
	if (!ContextMenuContainer)
	{
		return;
	}

>>>>>>> origin/main
	ActiveContextMenu = CreateWidget<UPDInventoryItemContextMenuWidget>(GetOwningPlayer(), ContextMenuWidgetClass);
	if (!ActiveContextMenu)
	{
		return;
	}

	ActiveContextMenu->OnUseClicked.AddUniqueDynamic(this, &UPDInventoryWidget::HandleContextMenuUseClicked);
	ActiveContextMenu->OnDropClicked.AddUniqueDynamic(this, &UPDInventoryWidget::HandleContextMenuDropClicked);
	ActiveContextMenu->OnEquipClicked.AddUniqueDynamic(this, &UPDInventoryWidget::HandleContextMenuEquipClicked);
	ActiveContextMenu->InitializeContextMenu(SlotIndex, SlotData);
<<<<<<< HEAD
	ActiveContextMenu->SetVisibility(ESlateVisibility::Visible);
	ActiveContextMenu->AddToViewport(300);

	FVector2D MousePosition = FVector2D::ZeroVector;
	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		UWidgetLayoutLibrary::GetMousePositionScaledByDPI(PlayerController, MousePosition.X, MousePosition.Y);
	}

	ActiveContextMenu->SetPositionInViewport(MousePosition, false);
=======
	ContextMenuContainer->ClearChildren();
	ContextMenuContainer->AddChild(ActiveContextMenu);
	ContextMenuContainer->SetVisibility(ESlateVisibility::Visible);
	ActiveContextMenu->ForceLayoutPrepass();
>>>>>>> origin/main
}

void UPDInventoryWidget::CloseContextMenu()
{
<<<<<<< HEAD
	if (!ActiveContextMenu)
	{
		return;
	}

	ActiveContextMenu->OnUseClicked.RemoveDynamic(this, &UPDInventoryWidget::HandleContextMenuUseClicked);
	ActiveContextMenu->OnDropClicked.RemoveDynamic(this, &UPDInventoryWidget::HandleContextMenuDropClicked);
	ActiveContextMenu->OnEquipClicked.RemoveDynamic(this, &UPDInventoryWidget::HandleContextMenuEquipClicked);
	ActiveContextMenu->RemoveFromParent();
	ActiveContextMenu = nullptr;
}

=======
	if (ActiveContextMenu)
	{
		ActiveContextMenu->OnUseClicked.RemoveDynamic(this, &UPDInventoryWidget::HandleContextMenuUseClicked);
		ActiveContextMenu->OnDropClicked.RemoveDynamic(this, &UPDInventoryWidget::HandleContextMenuDropClicked);
		ActiveContextMenu->OnEquipClicked.RemoveDynamic(this, &UPDInventoryWidget::HandleContextMenuEquipClicked);
		ActiveContextMenu->RemoveFromParent();
		ActiveContextMenu = nullptr;
	}

	if (UPanelWidget* ContextMenuContainer = FindContextMenuContainer())
	{
		ContextMenuContainer->ClearChildren();
		ContextMenuContainer->SetVisibility(ESlateVisibility::Collapsed);
	}
}

UPanelWidget* UPDInventoryWidget::FindContextMenuContainer() const
{
	if (!ActiveItemTooltip || !ActiveItemTooltip->WidgetTree)
	{
		return nullptr;
	}

	if (!ContextMenuContainerWidgetName.IsNone())
	{
		if (UPanelWidget* FoundPanel = Cast<UPanelWidget>(ActiveItemTooltip->WidgetTree->FindWidget(ContextMenuContainerWidgetName)))
		{
			return FoundPanel;
		}
	}

	if (UPanelWidget* FoundPanel = Cast<UPanelWidget>(ActiveItemTooltip->WidgetTree->FindWidget(TEXT("ContextMenuContainer"))))
	{
		return FoundPanel;
	}

	return nullptr;
}


>>>>>>> origin/main
void UPDInventoryWidget::OpenItemHoverTooltip(UPDInventorySlotWidget* SlotWidget)
{
	if (!SlotWidget || SlotWidget->GetSlotData().IsEmpty())
	{
		CloseItemHoverTooltip();
		return;
	}

	if (ActiveItemTooltip && ActiveTooltipSlot == SlotWidget)
	{
		ActiveTooltipPosition = GetSlotTooltipPosition(SlotWidget);
		ActiveItemTooltip->SetPositionInViewport(ActiveTooltipPosition, false);
		return;
	}

	CloseItemHoverTooltip();

	ActiveItemTooltip = SlotWidget->CreateItemTooltipWidget();
	if (!ActiveItemTooltip)
	{
		return;
	}

	ActiveTooltipSlot = SlotWidget;
	ActiveTooltipPosition = GetSlotTooltipPosition(SlotWidget);
	ActiveItemTooltip->AddToViewport(190);
	ActiveItemTooltip->ForceLayoutPrepass();
	ActiveItemTooltip->SetPositionInViewport(ActiveTooltipPosition, false);
}

void UPDInventoryWidget::CloseItemHoverTooltip()
{
	if (ActiveItemTooltip)
	{
		if (ActiveItemTooltip->IsInViewport())
		{
			ActiveItemTooltip->RemoveFromParent();
		}

		ActiveItemTooltip = nullptr;
	}

	ActiveTooltipSlot = nullptr;
	ActiveTooltipPosition = FVector2D::ZeroVector;
}

FVector2D UPDInventoryWidget::GetSlotTooltipPosition(UPDInventorySlotWidget* SlotWidget) const
{
	if (!SlotWidget)
	{
		return FVector2D::ZeroVector;
	}

	FVector2D PixelPosition;
	FVector2D ViewportPosition;
	const FGeometry& SlotGeometry = SlotWidget->GetCachedGeometry();
	const FVector2D SlotSize = SlotGeometry.GetLocalSize();
	USlateBlueprintLibrary::LocalToViewport(this, SlotGeometry, FVector2D(SlotSize.X, 0.0f), PixelPosition, ViewportPosition);

	return ViewportPosition + FVector2D(8.0f, 0.0f);
}

void UPDInventoryWidget::HandleContextMenuUseClicked(UPDInventoryItemContextMenuWidget* MenuWidget, int32 SlotIndex)
{
<<<<<<< HEAD
	UPDWidgetSoundLibrary::PlayUISound2D(this, ButtonClickSound);

	CloseContextMenu();
	CloseItemHoverTooltip();

	const FPDInventorySlot* SourceSlot = FindInventorySlot(SlotIndex);
	if (!SourceSlot || SourceSlot->IsEmpty())
	{
		return;
	}

	if (SourceSlot->ItemData.ItemType == EPDItemType::Consumable)
	{
		if (UPDQuickSlotComponent* QuickSlotComponent = FindQuickSlotComponent())
		{
			QuickSlotComponent->UseInventoryConsumableSlot(SlotIndex);
		}
=======
	CloseContextMenu();
	CloseItemHoverTooltip();

	if (UPDInventoryComponent* InventoryComponent = FindInventoryComponent())
	{
		InventoryComponent->UseItemFromSlot(SlotIndex);
>>>>>>> origin/main
	}
}

void UPDInventoryWidget::HandleContextMenuDropClicked(UPDInventoryItemContextMenuWidget* MenuWidget, int32 SlotIndex)
{
<<<<<<< HEAD
	UPDWidgetSoundLibrary::PlayUISound2D(this, ButtonClickSound);

=======
>>>>>>> origin/main
	CloseContextMenu();
	CloseItemHoverTooltip();

	if (UPDInventoryComponent* InventoryComponent = FindInventoryComponent())
	{
		InventoryComponent->DropItemFromSlot(SlotIndex, 1);
	}
}

void UPDInventoryWidget::HandleContextMenuEquipClicked(UPDInventoryItemContextMenuWidget* MenuWidget, int32 SlotIndex)
{
<<<<<<< HEAD
	UPDWidgetSoundLibrary::PlayUISound2D(this, ButtonClickSound);

	CloseContextMenu();
	CloseItemHoverTooltip();

	UPDInventoryComponent* InventoryComponent = FindInventoryComponent();
	UPDEquipmentComponent* EquipmentComponent = FindEquipmentComponent();
	if (!InventoryComponent || !EquipmentComponent)
	{
		return;
	}

	const FPDInventorySlot* SourceSlot = FindInventorySlot(SlotIndex);
	if (!SourceSlot || SourceSlot->IsEmpty())
	{
		return;
	}

	if (EquipmentComponent->ResolveEquipmentSlotType(SourceSlot->ItemData) == EPDEquipmentSlotType::Weapon)
	{
		if (UPDQuickSlotComponent* QuickSlotComponent = FindQuickSlotComponent())
		{
			if (QuickSlotComponent->EquipInventoryWeaponSlot(SlotIndex))
			{
				RefreshEquipmentSlots();
				RefreshInventoryGrid();
			}
		}
		return;
	}

	if (EquipmentComponent->EquipItemFromInventory(InventoryComponent, SlotIndex))
	{
		RefreshEquipmentSlots();
		RefreshInventoryGrid();
	}
=======
	CloseContextMenu();
	CloseItemHoverTooltip();
>>>>>>> origin/main
}

void UPDInventoryWidget::ExecuteInventoryQuickAction(int32 SlotIndex, int32 Quantity)
{
	APDPlayerController* PlayerController = Cast<APDPlayerController>(GetOwningPlayer());
	if (!PlayerController || Quantity <= 0)
	{
		return;
	}

	if (PlayerController->IsStashInterfaceOpen())
	{
		UPDInventoryComponent* InventoryComponent = FindInventoryComponent();
		UPDStashComponent* StashComponent = FindStashComponent();

		if (InventoryComponent && StashComponent)
		{
			StashComponent->StoreInventorySlotQuantity(InventoryComponent, SlotIndex, Quantity);
		}

		return;
	}

	if (PlayerController->IsMarketInterfaceOpen())
	{
		PlayerController->SellInventorySlotToActiveMarket(SlotIndex, Quantity);
	}
}


void UPDInventoryWidget::ExecuteInventorySlotTransfer(EPDItemContainerType SourceContainerType, int32 SourceSlotIndex, int32 TargetSlotIndex, int32 Quantity)
{
	if (Quantity <= 0)
	{
		return;
	}

	UPDInventoryComponent* InventoryComponent = FindInventoryComponent();
	if (!InventoryComponent)
	{
		return;
	}

	switch (SourceContainerType)
	{
	case EPDItemContainerType::Inventory:
		InventoryComponent->MoveSlotQuantityToSlot(SourceSlotIndex, TargetSlotIndex, Quantity);
		break;
	case EPDItemContainerType::Stash:
		if (UPDStashComponent* StashComponent = FindStashComponent())
		{
			StashComponent->TakeStashSlotQuantityToInventorySlot(InventoryComponent, SourceSlotIndex, TargetSlotIndex, Quantity);
		}
		break;
	case EPDItemContainerType::QuickSlot:
		if (UPDQuickSlotComponent* QuickSlotComponent = FindQuickSlotComponent())
		{
<<<<<<< HEAD
			QuickSlotComponent->RemoveItemFromSlot(SourceSlotIndex, Quantity);
		}
		break;
	case EPDItemContainerType::Equipment:
		if (UPDEquipmentComponent* EquipmentComponent = FindEquipmentComponent())
		{
			EquipmentComponent->UnequipItemToInventorySlot(InventoryComponent, static_cast<EPDEquipmentSlotType>(SourceSlotIndex), TargetSlotIndex);
		}
		break;
	case EPDItemContainerType::SecureContainer:
		if (UPDSecureContainerComponent* SecureContainerComponent = FindSecureContainerComponent())
		{
			SecureContainerComponent->TakeSecureSlotQuantityToInventorySlot(InventoryComponent, SourceSlotIndex, TargetSlotIndex, Quantity);
		}
		break;
	case EPDItemContainerType::Loot:
		if (APDPlayerController* PlayerController = Cast<APDPlayerController>(GetOwningPlayer()))
		{
			if (UPDLootComponent* LootComponent = PlayerController->GetActiveLootComponent())
			{
				LootComponent->TakeSlotQuantityToInventorySlot(InventoryComponent, SourceSlotIndex, TargetSlotIndex, Quantity);
			}
=======
			QuickSlotComponent->TakeQuickSlotQuantityToInventorySlot(InventoryComponent, SourceSlotIndex, TargetSlotIndex, Quantity);
>>>>>>> origin/main
		}
		break;
	default:
		break;
	}
}

bool UPDInventoryWidget::ShouldOpenTransferQuantityPopup(EPDItemContainerType SourceContainerType, int32 SourceSlotIndex, int32 TargetSlotIndex, int32& OutMaxQuantity, FText& OutTitle) const
{
	OutMaxQuantity = 0;
	OutTitle = FText::GetEmpty();

	if (SourceContainerType == EPDItemContainerType::Inventory)
	{
		return false;
	}

	const FPDInventorySlot* SourceSlot = FindSourceSlot(SourceContainerType, SourceSlotIndex);
	const FPDInventorySlot* TargetSlot = FindInventorySlot(TargetSlotIndex);
	if (!SourceSlot || !TargetSlot || SourceSlot->IsEmpty())
	{
		return false;
	}

	OutMaxQuantity = FPDItemSlotTransfer::GetQuantityLimit(*SourceSlot, *TargetSlot);
	OutTitle = SourceSlot->ItemData.DisplayName.IsEmpty() ? FText::FromName(SourceSlot->ItemData.ItemID) : SourceSlot->ItemData.DisplayName;
	return FPDItemSlotTransfer::CanPromptForPartial(*SourceSlot, *TargetSlot) && OutMaxQuantity > 1;
}

void UPDInventoryWidget::OpenQuantityPopup(int32 SlotIndex, int32 MaxQuantity, const FText& Title)
{
	if (!QuantityPopupWidgetClass)
	{
		ExecuteInventoryQuickAction(SlotIndex, MaxQuantity);
		return;
	}

	if (ActiveQuantityPopup && ActiveQuantityPopup->IsInViewport())
	{
		ActiveQuantityPopup->RemoveFromParent();
	}

	ActiveQuantityPopup = CreateWidget<UPDQuantityPopupWidget>(GetOwningPlayer(), QuantityPopupWidgetClass);
	if (!ActiveQuantityPopup)
	{
		ExecuteInventoryQuickAction(SlotIndex, MaxQuantity);
		return;
	}

	bPendingTransferQuantityRequest = false;
	PendingSlotIndex = SlotIndex;
	ActiveQuantityPopup->OnConfirmed.RemoveDynamic(this, &UPDInventoryWidget::HandleQuantityConfirmed);
	ActiveQuantityPopup->OnConfirmed.AddUniqueDynamic(this, &UPDInventoryWidget::HandleQuantityConfirmed);
	ActiveQuantityPopup->OnCancelled.RemoveDynamic(this, &UPDInventoryWidget::HandleQuantityCancelled);
	ActiveQuantityPopup->OnCancelled.AddUniqueDynamic(this, &UPDInventoryWidget::HandleQuantityCancelled);
<<<<<<< HEAD
=======
	ActiveQuantityPopup->AddToViewport(100);
	ActiveQuantityPopup->InitializeQuantityPopup(MaxQuantity, Title);
}

void UPDInventoryWidget::OpenTransferQuantityPopup(EPDItemContainerType SourceContainerType, int32 SourceSlotIndex, int32 TargetSlotIndex, int32 MaxQuantity, const FText& Title)
{
	if (!QuantityPopupWidgetClass)
	{
		ExecuteInventorySlotTransfer(SourceContainerType, SourceSlotIndex, TargetSlotIndex, MaxQuantity);
		return;
	}

	if (ActiveQuantityPopup && ActiveQuantityPopup->IsInViewport())
	{
		ActiveQuantityPopup->RemoveFromParent();
	}

	ActiveQuantityPopup = CreateWidget<UPDQuantityPopupWidget>(GetOwningPlayer(), QuantityPopupWidgetClass);
	if (!ActiveQuantityPopup)
	{
		ExecuteInventorySlotTransfer(SourceContainerType, SourceSlotIndex, TargetSlotIndex, MaxQuantity);
		return;
	}

	bPendingTransferQuantityRequest = true;
	PendingTransferSourceContainerType = SourceContainerType;
	PendingTransferSourceSlotIndex = SourceSlotIndex;
	PendingTransferTargetSlotIndex = TargetSlotIndex;
	PendingSlotIndex = INDEX_NONE;
	ActiveQuantityPopup->OnConfirmed.RemoveDynamic(this, &UPDInventoryWidget::HandleQuantityConfirmed);
	ActiveQuantityPopup->OnConfirmed.AddUniqueDynamic(this, &UPDInventoryWidget::HandleQuantityConfirmed);
	ActiveQuantityPopup->OnCancelled.RemoveDynamic(this, &UPDInventoryWidget::HandleQuantityCancelled);
	ActiveQuantityPopup->OnCancelled.AddUniqueDynamic(this, &UPDInventoryWidget::HandleQuantityCancelled);
>>>>>>> origin/main
	ActiveQuantityPopup->AddToViewport(100);
	if (const FPDInventorySlot* PreviewSlot = FindInventorySlot(SlotIndex))
	{
		ActiveQuantityPopup->InitializeQuantityPopupWithSlot(MaxQuantity, Title, *PreviewSlot);
	}
	else
	{
		ActiveQuantityPopup->InitializeQuantityPopup(MaxQuantity, Title);
	}
}

void UPDInventoryWidget::OpenTransferQuantityPopup(EPDItemContainerType SourceContainerType, int32 SourceSlotIndex, int32 TargetSlotIndex, int32 MaxQuantity, const FText& Title)
{
	if (!QuantityPopupWidgetClass)
	{
		ExecuteInventorySlotTransfer(SourceContainerType, SourceSlotIndex, TargetSlotIndex, MaxQuantity);
		return;
	}

	if (ActiveQuantityPopup && ActiveQuantityPopup->IsInViewport())
	{
		ActiveQuantityPopup->RemoveFromParent();
	}

	ActiveQuantityPopup = CreateWidget<UPDQuantityPopupWidget>(GetOwningPlayer(), QuantityPopupWidgetClass);
	if (!ActiveQuantityPopup)
	{
		ExecuteInventorySlotTransfer(SourceContainerType, SourceSlotIndex, TargetSlotIndex, MaxQuantity);
		return;
	}

	bPendingTransferQuantityRequest = true;
	PendingTransferSourceContainerType = SourceContainerType;
	PendingTransferSourceSlotIndex = SourceSlotIndex;
	PendingTransferTargetSlotIndex = TargetSlotIndex;
	PendingSlotIndex = INDEX_NONE;
	ActiveQuantityPopup->OnConfirmed.RemoveDynamic(this, &UPDInventoryWidget::HandleQuantityConfirmed);
	ActiveQuantityPopup->OnConfirmed.AddUniqueDynamic(this, &UPDInventoryWidget::HandleQuantityConfirmed);
	ActiveQuantityPopup->OnCancelled.RemoveDynamic(this, &UPDInventoryWidget::HandleQuantityCancelled);
	ActiveQuantityPopup->OnCancelled.AddUniqueDynamic(this, &UPDInventoryWidget::HandleQuantityCancelled);
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

void UPDInventoryWidget::HandleQuantityConfirmed(int32 Quantity)
{
	if (bPendingTransferQuantityRequest)
	{
		ExecuteInventorySlotTransfer(PendingTransferSourceContainerType, PendingTransferSourceSlotIndex, PendingTransferTargetSlotIndex, Quantity);
	}
	else if (PendingSlotIndex != INDEX_NONE)
	{
		ExecuteInventoryQuickAction(PendingSlotIndex, Quantity);
	}

	ClearQuantityRequest();
}

void UPDInventoryWidget::HandleQuantityCancelled()
{
	ClearQuantityRequest();
}

void UPDInventoryWidget::ClearQuantityRequest()
{
	bPendingTransferQuantityRequest = false;
	PendingTransferSourceContainerType = EPDItemContainerType::None;
	PendingTransferSourceSlotIndex = INDEX_NONE;
	PendingTransferTargetSlotIndex = INDEX_NONE;
	PendingSlotIndex = INDEX_NONE;
	ActiveQuantityPopup = nullptr;
}

void UPDInventoryWidget::BindInventoryChanged()
{
	UPDInventoryComponent* InventoryComponent = FindInventoryComponent();

	if (BoundInventoryComponent == InventoryComponent)
	{
		return;
	}

	UnbindInventoryChanged();

	BoundInventoryComponent = InventoryComponent;

	if (BoundInventoryComponent)
	{
		BoundInventoryComponent->OnInventoryChanged.AddUniqueDynamic(this, &UPDInventoryWidget::RefreshInventoryGrid);
		BoundInventoryComponent->OnInventoryWeightLimitExceeded.AddUniqueDynamic(this, &UPDInventoryWidget::HandleInventoryWeightLimitExceeded);
	}
}

void UPDInventoryWidget::UnbindInventoryChanged()
{
	if (BoundInventoryComponent)
	{
		BoundInventoryComponent->OnInventoryChanged.RemoveDynamic(this, &UPDInventoryWidget::RefreshInventoryGrid);
		BoundInventoryComponent->OnInventoryWeightLimitExceeded.RemoveDynamic(this, &UPDInventoryWidget::HandleInventoryWeightLimitExceeded);
		BoundInventoryComponent = nullptr;
	}
}
