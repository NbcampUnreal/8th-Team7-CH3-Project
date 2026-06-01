#include "Widgets/Inventory/PDEquipmentModificationWidget.h"
#include "Widgets/PDWidgetSoundLibrary.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBox.h"
#include "Core/PDPlayerComponentResolver.h"
#include "GameFramework/Pawn.h"
#include "Items/Equipment/PDEquipmentModificationComponent.h"
#include "Items/Containers/PDInventoryComponent.h"
#include "Widgets/Inventory/PDEquipmentListItemWidget.h"
#include "Widgets/Inventory/PDInventorySlotWidget.h"
#include "Widgets/Inventory/PDInventoryDragDropOperation.h"

void UPDEquipmentModificationWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ResolveComponents();
	BindWidgetEvents();
	BindComponentEvents();
	RefreshAll();
}

void UPDEquipmentModificationWidget::NativeDestruct()
{
	UnbindWidgetEvents();
	UnbindComponentEvents();
	Super::NativeDestruct();
}

void UPDEquipmentModificationWidget::ResolveComponents()
{
	APawn* OwningPawn = GetOwningPlayerPawn();

	// 2�?구조: InventoryComponent??PlayerState??존재, ModificationComponent??PlayerCharacter(=Pawn)??존재.
	InventoryComponent = FPDPlayerComponentResolver::ResolveInventory(GetOwningPlayer());
	if (!InventoryComponent)
	{
		InventoryComponent = FPDPlayerComponentResolver::ResolveInventory(OwningPawn);
	}
	ModificationComponent = OwningPawn ? OwningPawn->FindComponentByClass<UPDEquipmentModificationComponent>() : nullptr;

	if (!BoostSlotWidget && BoostMaterialSlotWidget)
	{
		BoostSlotWidget = BoostMaterialSlotWidget;
	}
}

void UPDEquipmentModificationWidget::BindWidgetEvents()
{
	if (Button_EquipmentTab)
	{
		Button_EquipmentTab->OnClicked.AddUniqueDynamic(this, &UPDEquipmentModificationWidget::HandleEquipmentTabClicked);
	}
	if (Button_MaterialTab)
	{
		Button_MaterialTab->OnClicked.AddUniqueDynamic(this, &UPDEquipmentModificationWidget::HandleMaterialTabClicked);
	}
	if (Button_Boost_None)
	{
		Button_Boost_None->OnClicked.AddUniqueDynamic(this, &UPDEquipmentModificationWidget::HandleBoostNoneClicked);
	}
	if (Button_Boost_Low)
	{
		Button_Boost_Low->OnClicked.AddUniqueDynamic(this, &UPDEquipmentModificationWidget::HandleBoostLowClicked);
	}
	if (Button_Boost_Mid)
	{
		Button_Boost_Mid->OnClicked.AddUniqueDynamic(this, &UPDEquipmentModificationWidget::HandleBoostMidClicked);
	}
	if (Button_Boost_High)
	{
		Button_Boost_High->OnClicked.AddUniqueDynamic(this, &UPDEquipmentModificationWidget::HandleBoostHighClicked);
	}
	if (Button_Modify)
	{
		Button_Modify->OnClicked.AddUniqueDynamic(this, &UPDEquipmentModificationWidget::HandleModifyClicked);
	}
	if (BTN_Enhance && BTN_Enhance != Button_Modify)
	{
		BTN_Enhance->OnClicked.AddUniqueDynamic(this, &UPDEquipmentModificationWidget::HandleModifyClicked);
	}
	if (EquipmentSlotWidget)
	{
		EquipmentSlotWidget->OnSlotLeftClicked.AddUniqueDynamic(this, &UPDEquipmentModificationWidget::HandleInventorySlotClicked);
		EquipmentSlotWidget->OnSlotItemDropped.AddUniqueDynamic(this, &UPDEquipmentModificationWidget::HandleSelectionSlotItemDropped);
	}
	if (MaterialSlotWidget)
	{
		MaterialSlotWidget->OnSlotLeftClicked.AddUniqueDynamic(this, &UPDEquipmentModificationWidget::HandleInventorySlotClicked);
		MaterialSlotWidget->OnSlotItemDropped.AddUniqueDynamic(this, &UPDEquipmentModificationWidget::HandleSelectionSlotItemDropped);
	}
	if (BoostSlotWidget)
	{
		BoostSlotWidget->OnSlotLeftClicked.AddUniqueDynamic(this, &UPDEquipmentModificationWidget::HandleInventorySlotClicked);
		BoostSlotWidget->OnSlotItemDropped.AddUniqueDynamic(this, &UPDEquipmentModificationWidget::HandleSelectionSlotItemDropped);
	}
}

void UPDEquipmentModificationWidget::UnbindWidgetEvents()
{
	if (Button_EquipmentTab)
	{
		Button_EquipmentTab->OnClicked.RemoveDynamic(this, &UPDEquipmentModificationWidget::HandleEquipmentTabClicked);
	}
	if (Button_MaterialTab)
	{
		Button_MaterialTab->OnClicked.RemoveDynamic(this, &UPDEquipmentModificationWidget::HandleMaterialTabClicked);
	}
	if (Button_Boost_None)
	{
		Button_Boost_None->OnClicked.RemoveDynamic(this, &UPDEquipmentModificationWidget::HandleBoostNoneClicked);
	}
	if (Button_Boost_Low)
	{
		Button_Boost_Low->OnClicked.RemoveDynamic(this, &UPDEquipmentModificationWidget::HandleBoostLowClicked);
	}
	if (Button_Boost_Mid)
	{
		Button_Boost_Mid->OnClicked.RemoveDynamic(this, &UPDEquipmentModificationWidget::HandleBoostMidClicked);
	}
	if (Button_Boost_High)
	{
		Button_Boost_High->OnClicked.RemoveDynamic(this, &UPDEquipmentModificationWidget::HandleBoostHighClicked);
	}
	if (Button_Modify)
	{
		Button_Modify->OnClicked.RemoveDynamic(this, &UPDEquipmentModificationWidget::HandleModifyClicked);
	}
	if (BTN_Enhance && BTN_Enhance != Button_Modify)
	{
		BTN_Enhance->OnClicked.RemoveDynamic(this, &UPDEquipmentModificationWidget::HandleModifyClicked);
	}
	if (EquipmentSlotWidget)
	{
		EquipmentSlotWidget->OnSlotLeftClicked.RemoveDynamic(this, &UPDEquipmentModificationWidget::HandleInventorySlotClicked);
		EquipmentSlotWidget->OnSlotItemDropped.RemoveDynamic(this, &UPDEquipmentModificationWidget::HandleSelectionSlotItemDropped);
	}
	if (MaterialSlotWidget)
	{
		MaterialSlotWidget->OnSlotLeftClicked.RemoveDynamic(this, &UPDEquipmentModificationWidget::HandleInventorySlotClicked);
		MaterialSlotWidget->OnSlotItemDropped.RemoveDynamic(this, &UPDEquipmentModificationWidget::HandleSelectionSlotItemDropped);
	}
	if (BoostSlotWidget)
	{
		BoostSlotWidget->OnSlotLeftClicked.RemoveDynamic(this, &UPDEquipmentModificationWidget::HandleInventorySlotClicked);
		BoostSlotWidget->OnSlotItemDropped.RemoveDynamic(this, &UPDEquipmentModificationWidget::HandleSelectionSlotItemDropped);
	}
}

void UPDEquipmentModificationWidget::BindComponentEvents()
{
	if (InventoryComponent)
	{
		InventoryComponent->OnInventoryChanged.AddUniqueDynamic(this, &UPDEquipmentModificationWidget::HandleInventoryChanged);
	}
	if (ModificationComponent)
	{
		ModificationComponent->OnModificationFinished.AddUniqueDynamic(this, &UPDEquipmentModificationWidget::HandleModificationFinished);
	}
}

void UPDEquipmentModificationWidget::UnbindComponentEvents()
{
	if (InventoryComponent)
	{
		InventoryComponent->OnInventoryChanged.RemoveDynamic(this, &UPDEquipmentModificationWidget::HandleInventoryChanged);
	}
	if (ModificationComponent)
	{
		ModificationComponent->OnModificationFinished.RemoveDynamic(this, &UPDEquipmentModificationWidget::HandleModificationFinished);
	}
}

void UPDEquipmentModificationWidget::RefreshAll()
{
	RefreshEquipmentList();
	SelectFirstEquipmentSlotIfNeeded();
	RefreshSelectedSlots();
	RefreshInventoryGrid();
	RefreshPreview();
	SetText(Text_PlayerGold, FormatGoldAmount(InventoryComponent ? InventoryComponent->GetGold() : 0));
}

void UPDEquipmentModificationWidget::RefreshEquipmentList()
{
	if (!ScrollBox_EquipmentList)
	{
		return;
	}

	ScrollBox_EquipmentList->ClearChildren();

	if (!InventoryComponent || !EquipmentListItemWidgetClass)
	{
		return;
	}

	for (int32 Index = 0; Index < InventoryComponent->Items.Num(); ++Index)
	{
		const FPDInventorySlot& InventorySlotData = InventoryComponent->Items[Index];
		if (!IsValidEquipmentSlotIndex(Index))
		{
			continue;
		}

		UPDEquipmentListItemWidget* ItemWidget = CreateWidget<UPDEquipmentListItemWidget>(GetOwningPlayer(), EquipmentListItemWidgetClass);
		if (!ItemWidget)
		{
			continue;
		}

		ItemWidget->SetSlotData(InventorySlotData, Index);
		ItemWidget->SetSelected(Index == SelectedSlotIndex);
		ItemWidget->OnItemClicked.AddUniqueDynamic(this, &UPDEquipmentModificationWidget::HandleEquipmentListItemClicked);
		ScrollBox_EquipmentList->AddChild(ItemWidget);
	}
}

void UPDEquipmentModificationWidget::RefreshInventoryGrid()
{
	if (!InventoryGrid)
	{
		return;
	}

	InventoryGrid->ClearChildren();

	if (!InventorySlotWidgetClass)
	{
		return;
	}

	const int32 SafeColumns = FMath::Max(1, InventoryGridColumns);
	const int32 SafeRows = FMath::Max(1, InventoryGridRows);
	const int32 MaxVisibleSlots = SafeColumns * SafeRows;
	TArray<int32> DisplaySlotIndices;
	DisplaySlotIndices.Reserve(MaxVisibleSlots);

	if (InventoryComponent)
	{
		for (int32 SlotIndex = 0; SlotIndex < InventoryComponent->Items.Num() && DisplaySlotIndices.Num() < MaxVisibleSlots; ++SlotIndex)
		{
			const FPDInventorySlot& SlotData = InventoryComponent->Items[SlotIndex];
			if (SlotData.IsEmpty())
			{
				continue;
			}

			if (CurrentInventoryFilter == EPDItemType::Equipment && IsValidEquipmentSlotIndex(SlotIndex))
			{
				DisplaySlotIndices.Add(SlotIndex);
			}
			else if (CurrentInventoryFilter == EPDItemType::Misc && (IsValidMaterialSlotIndex(SlotIndex) || IsRegisteredBoostItem(SlotData.ItemData.ItemID)))
			{
				DisplaySlotIndices.Add(SlotIndex);
			}
		}
	}

	for (int32 DisplayIndex = 0; DisplayIndex < MaxVisibleSlots; ++DisplayIndex)
	{
		UPDInventorySlotWidget* SlotWidget = CreateWidget<UPDInventorySlotWidget>(GetOwningPlayer(), InventorySlotWidgetClass);
		if (!SlotWidget)
		{
			continue;
		}

		SlotWidget->SetSlotContainerType(EPDItemContainerType::Inventory);
		SlotWidget->OnSlotLeftClicked.AddUniqueDynamic(this, &UPDEquipmentModificationWidget::HandleInventorySlotClicked);

		if (DisplaySlotIndices.IsValidIndex(DisplayIndex))
		{
			const int32 SlotIndex = DisplaySlotIndices[DisplayIndex];
			SlotWidget->SetSlotData(InventoryComponent->Items[SlotIndex], SlotIndex);
		}
		else
		{
			SlotWidget->ClearSlotData(INDEX_NONE);
		}

		UWidget* GridChild = SlotWidget;

		if (InventoryGridSlotPadding > 0.f)
		{
			UBorder* SlotPaddingWrapper = NewObject<UBorder>(this);
			if (SlotPaddingWrapper)
			{
				SlotPaddingWrapper->SetBrushColor(FLinearColor::Transparent);
				SlotPaddingWrapper->SetPadding(FMargin(InventoryGridSlotPadding));
				SlotPaddingWrapper->AddChild(SlotWidget);
				GridChild = SlotPaddingWrapper;
			}
		}

		UUniformGridSlot* GridSlot = InventoryGrid->AddChildToUniformGrid(GridChild, DisplayIndex / SafeColumns, DisplayIndex % SafeColumns);
		if (GridSlot)
		{
			GridSlot->SetHorizontalAlignment(HAlign_Left);
			GridSlot->SetVerticalAlignment(VAlign_Top);
		}
	}
}

void UPDEquipmentModificationWidget::SelectFirstEquipmentSlotIfNeeded()
{
	if (SelectedSlotIndex != INDEX_NONE || !InventoryComponent)
	{
		return;
	}

	for (int32 Index = 0; Index < InventoryComponent->Items.Num(); ++Index)
	{
		if (IsValidEquipmentSlotIndex(Index))
		{
			SelectedSlotIndex = Index;
			return;
		}
	}
}

void UPDEquipmentModificationWidget::SelectInventorySlot(int32 SlotIndex)
{
	if (!IsValidEquipmentSlotIndex(SlotIndex))
	{
		return;
	}

	SelectedSlotIndex = SlotIndex;
	SetText(Text_Result, ModifyReadyText);
	RefreshEquipmentList();
	RefreshSelectedSlots();
	RefreshPreview();
}

void UPDEquipmentModificationWidget::SelectMaterialSlot(int32 SlotIndex)
{
	if (!IsValidMaterialSlotIndex(SlotIndex))
	{
		return;
	}

	SelectedMaterialSlotIndex = SlotIndex;
	SetText(Text_Result, ModifyReadyText);
	RefreshSelectedSlots();
	RefreshPreview();
}

void UPDEquipmentModificationWidget::SelectBoostSlot(int32 SlotIndex)
{
	if (!InventoryComponent || !InventoryComponent->Items.IsValidIndex(SlotIndex) || InventoryComponent->Items[SlotIndex].IsEmpty() || !IsRegisteredBoostItem(InventoryComponent->Items[SlotIndex].ItemData.ItemID))
	{
		return;
	}

	SelectedBoostSlotIndex = SlotIndex;
	SetText(Text_Result, ModifyReadyText);
	RefreshSelectedSlots();
	RefreshPreview();
}

void UPDEquipmentModificationWidget::SetInventoryFilter(EPDItemType ItemType)
{
	CurrentInventoryFilter = ItemType;
	RefreshInventoryGrid();
}

void UPDEquipmentModificationWidget::SetBoostType(EPDModificationBoostType BoostType)
{
	SelectedBoostType = BoostType;
	SetText(Text_Result, ModifyReadyText);
	RefreshPreview();
}

void UPDEquipmentModificationWidget::RefreshSelectedSlots()
{
	SetSlotWidgetData(EquipmentSlotWidget, SelectedSlotIndex, EmptySelectionText);
	SetSlotWidgetData(MaterialSlotWidget, SelectedMaterialSlotIndex, EmptyMaterialText);
	SetSlotWidgetData(BoostSlotWidget, SelectedBoostSlotIndex, FText::FromString(TEXT("Select boost material.")));
}

void UPDEquipmentModificationWidget::RefreshPreview()
{
	bCurrentPreviewValid = false;
	CurrentPreview = FPDModificationPreview();
	CurrentPreviewResult = EPDModificationResult::InvalidSlot;

	if (!InventoryComponent || !ModificationComponent || !InventoryComponent->Items.IsValidIndex(SelectedSlotIndex))
	{
		ClearPreview();
		return;
	}

	const FPDInventorySlot& InventorySlotData = InventoryComponent->Items[SelectedSlotIndex];
	if (InventorySlotData.IsEmpty() || InventorySlotData.ItemData.ItemType != EPDItemType::Equipment)
	{
		ClearPreview();
		return;
	}

	bCurrentPreviewValid = ModificationComponent->CanModifyInventorySlotWithMaterialAndBoostSlots(InventoryComponent, SelectedSlotIndex, SelectedMaterialSlotIndex, SelectedBoostSlotIndex, CurrentPreview, CurrentPreviewResult);
	if (!bCurrentPreviewValid)
	{
		EPDModificationBoostType PreviewBoostType = EPDModificationBoostType::None;
		FName PreviewBoostItemID = NAME_None;
		int32 PreviewBoostQuantity = 0;
		ModificationComponent->IsValidBoostSlot(InventoryComponent, SelectedBoostSlotIndex, PreviewBoostType, PreviewBoostItemID, PreviewBoostQuantity);
		ModificationComponent->GetModificationPreviewWithBoost(InventorySlotData, PreviewBoostType, CurrentPreview, CurrentPreviewResult);
		if (!PreviewBoostItemID.IsNone())
		{
			CurrentPreview.SelectedBoostType = PreviewBoostType;
			CurrentPreview.BoostItemID = PreviewBoostItemID;
			CurrentPreview.BoostItemQuantity = PreviewBoostQuantity;
		}
	}

	ApplyPreview(CurrentPreview, InventorySlotData);
}

void UPDEquipmentModificationWidget::ClearPreview()
{
	SetText(Text_SelectedItemName, EmptySelectionText);
	SetText(Text_CurrentLevel, FText::FromString(TEXT("-")));
	SetText(Text_TargetLevel, FText::FromString(TEXT("-")));
	SetText(Text_StatType, FText::FromString(TEXT("-")));
	SetText(Text_StatPreview, FText::FromString(TEXT("-")));
	SetText(Text_GoldCost, FText::FromString(TEXT("-")));
	SetText(Text_SuccessRate, FText::FromString(TEXT("-")));
	SetText(Text_RequiredGold, FText::FromString(TEXT("-")));
	SetText(Text_BaseSuccessRate, FText::FromString(TEXT("-")));
	SetText(Text_BoostSuccessRate, FText::FromString(TEXT("-")));
	SetText(Text_FinalSuccessRate, FText::FromString(TEXT("-")));
	if (Image_SelectedItemIcon)
	{
		Image_SelectedItemIcon->SetBrushFromTexture(nullptr);
	}
	if (VerticalBox_RequiredMaterials)
	{
		VerticalBox_RequiredMaterials->ClearChildren();
	}
	if (Button_Modify)
	{
		Button_Modify->SetIsEnabled(false);
	}
	if (BTN_Enhance)
	{
		BTN_Enhance->SetIsEnabled(false);
	}
}

void UPDEquipmentModificationWidget::ApplyPreview(const FPDModificationPreview& Preview, const FPDInventorySlot& SlotData)
{
	const FPDItemData& ItemData = SlotData.ItemData;
	SetText(Text_SelectedItemName, ItemData.DisplayName.IsEmpty() ? FText::FromName(ItemData.ItemID) : ItemData.DisplayName);
	SetText(Text_CurrentLevel, FormatLevelText(Preview.CurrentModificationLevel));

	if (CurrentPreviewResult == EPDModificationResult::AlreadyMaxLevel)
	{
		SetText(Text_TargetLevel, FText::FromString(TEXT("MAX")));
	}
	else
	{
		SetText(Text_TargetLevel, FormatLevelText(Preview.TargetModificationLevel));
	}

	if (Image_SelectedItemIcon)
	{
		Image_SelectedItemIcon->SetBrushFromTexture(ItemData.Icon);
	}

	if (Preview.AttackBonus > 0.f)
	{
		SetText(Text_StatType, FText::FromString(TEXT("Attack")));
		SetText(Text_StatPreview, FText::FromString(FString::Printf(TEXT("+%.1f"), Preview.AttackBonus)));
	}
	else if (Preview.DefenseBonus > 0.f)
	{
		SetText(Text_StatType, FText::FromString(TEXT("Defense")));
		SetText(Text_StatPreview, FText::FromString(FString::Printf(TEXT("+%.1f"), Preview.DefenseBonus)));
	}
	else
	{
		SetText(Text_StatType, FText::FromString(TEXT("-")));
		SetText(Text_StatPreview, FText::FromString(TEXT("-")));
	}

	SetText(Text_GoldCost, FText::Format(NSLOCTEXT("PDEquipmentModification", "GoldCostFormat", "{0} / {1}"), FormatGoldAmount(InventoryComponent ? InventoryComponent->GetGold() : 0), FormatGoldAmount(Preview.GoldCost)));
	SetText(Text_RequiredGold, FormatGoldAmount(Preview.GoldCost));
	SetText(Text_SuccessRate, FormatPercent(Preview.SuccessRate));
	SetText(Text_BaseSuccessRate, FormatPercent(Preview.BaseSuccessRate));
	SetText(Text_BoostSuccessRate, FText::FromString(FString::Printf(TEXT("+%s"), *FormatPercent(Preview.BoostSuccessRate).ToString())));
	SetText(Text_FinalSuccessRate, FormatPercent(Preview.SuccessRate));
	RefreshMaterialList(Preview);

	if (Button_Modify)
	{
		Button_Modify->SetIsEnabled(bCurrentPreviewValid);
	}
	if (BTN_Enhance)
	{
		BTN_Enhance->SetIsEnabled(bCurrentPreviewValid);
	}
}

void UPDEquipmentModificationWidget::RefreshMaterialList(const FPDModificationPreview& Preview)
{
	if (!VerticalBox_RequiredMaterials || !WidgetTree)
	{
		return;
	}

	VerticalBox_RequiredMaterials->ClearChildren();

	for (const FPDModificationMaterialRequirement& Material : Preview.RequiredMaterials)
	{
		UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		if (!TextBlock)
		{
			continue;
		}

		TextBlock->SetText(FText::FromString(FString::Printf(TEXT("%s  %d / %d"), *Material.RequiredItemID.ToString(), CountItem(Material.RequiredItemID), Material.Quantity)));
		VerticalBox_RequiredMaterials->AddChild(TextBlock);
	}

	if (!Preview.BoostItemID.IsNone() && Preview.BoostItemQuantity > 0)
	{
		UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		if (TextBlock)
		{
			TextBlock->SetText(FText::FromString(FString::Printf(TEXT("%s  %d / %d"), *Preview.BoostItemID.ToString(), CountItem(Preview.BoostItemID), Preview.BoostItemQuantity)));
			VerticalBox_RequiredMaterials->AddChild(TextBlock);
		}
	}
}
bool UPDEquipmentModificationWidget::TryAutoSelectInventoryItem(int32 SlotIndex)
{
	if (!InventoryComponent || !InventoryComponent->Items.IsValidIndex(SlotIndex))
	{
		return false;
	}

	const FPDInventorySlot& SlotData = InventoryComponent->Items[SlotIndex];
	if (SlotData.IsEmpty())
	{
		return false;
	}

	if (IsValidEquipmentSlotIndex(SlotIndex))
	{
		if (SelectedSlotIndex == SlotIndex)
		{
			SelectedSlotIndex = INDEX_NONE;
			SetText(Text_Result, ModifyReadyText);
			RefreshEquipmentList();
			RefreshSelectedSlots();
			RefreshPreview();
		}
		else
		{
			SelectInventorySlot(SlotIndex);
		}
		return true;
	}

	if (IsRegisteredBoostItem(SlotData.ItemData.ItemID))
	{
		if (SelectedBoostSlotIndex == SlotIndex)
		{
			SelectedBoostSlotIndex = INDEX_NONE;
			SetText(Text_Result, ModifyReadyText);
			RefreshSelectedSlots();
			RefreshPreview();
		}
		else
		{
			SelectBoostSlot(SlotIndex);
		}
		SetInventoryFilter(EPDItemType::Misc);
		return true;
	}

	if (IsValidMaterialSlotIndex(SlotIndex))
	{
		if (SelectedMaterialSlotIndex == SlotIndex)
		{
			SelectedMaterialSlotIndex = INDEX_NONE;
			SetText(Text_Result, ModifyReadyText);
			RefreshSelectedSlots();
			RefreshPreview();
		}
		else
		{
			SelectMaterialSlot(SlotIndex);
		}
		SetInventoryFilter(EPDItemType::Misc);
		return true;
	}

	return false;
}

void UPDEquipmentModificationWidget::HandleEquipmentListItemClicked(UPDEquipmentListItemWidget* ItemWidget, int32 SlotIndex)
{
	SelectInventorySlot(SlotIndex);
}

void UPDEquipmentModificationWidget::HandleInventorySlotClicked(UPDInventorySlotWidget* SlotWidget, int32 SlotIndex)
{
	if (SlotWidget == EquipmentSlotWidget)
	{
		SelectedSlotIndex = INDEX_NONE;
		SetText(Text_Result, ModifyReadyText);
		RefreshEquipmentList();
		RefreshSelectedSlots();
		RefreshPreview();
		return;
	}

	if (SlotWidget == MaterialSlotWidget)
	{
		SelectedMaterialSlotIndex = INDEX_NONE;
		SetText(Text_Result, ModifyReadyText);
		RefreshSelectedSlots();
		RefreshPreview();
		return;
	}

	if (SlotWidget == BoostSlotWidget)
	{
		SelectedBoostSlotIndex = INDEX_NONE;
		SetText(Text_Result, ModifyReadyText);
		RefreshSelectedSlots();
		RefreshPreview();
		return;
	}

	TryAutoSelectInventoryItem(SlotIndex);
}


void UPDEquipmentModificationWidget::HandleSelectionSlotItemDropped(UPDInventorySlotWidget* SlotWidget, int32 SlotIndex, UPDInventoryDragDropOperation* DragOperation)
{
	if (!DragOperation || !DragOperation->IsValidPayload() || !InventoryComponent || !InventoryComponent->Items.IsValidIndex(DragOperation->SourceSlotIndex))
	{
		return;
	}

	const int32 SourceSlotIndex = DragOperation->SourceSlotIndex;
	if (SlotWidget == EquipmentSlotWidget)
	{
		SelectInventorySlot(SourceSlotIndex);
		return;
	}

	if (SlotWidget == MaterialSlotWidget)
	{
		if (IsRegisteredBoostItem(InventoryComponent->Items[SourceSlotIndex].ItemData.ItemID))
		{
			SelectBoostSlot(SourceSlotIndex);
		}
		else
		{
			SelectMaterialSlot(SourceSlotIndex);
		}
		return;
	}

	if (SlotWidget == BoostSlotWidget)
	{
		if (IsRegisteredBoostItem(InventoryComponent->Items[SourceSlotIndex].ItemData.ItemID))
		{
			SelectBoostSlot(SourceSlotIndex);
		}
	}
}

void UPDEquipmentModificationWidget::HandleEquipmentTabClicked()
{
	UPDWidgetSoundLibrary::PlayUISound2D(this, ButtonClickSound);

	SetInventoryFilter(EPDItemType::Equipment);
}

void UPDEquipmentModificationWidget::HandleMaterialTabClicked()
{
	UPDWidgetSoundLibrary::PlayUISound2D(this, ButtonClickSound);

	SetInventoryFilter(EPDItemType::Misc);
}

void UPDEquipmentModificationWidget::HandleBoostNoneClicked()
{
	UPDWidgetSoundLibrary::PlayUISound2D(this, ButtonClickSound);

	SetBoostType(EPDModificationBoostType::None);
}

void UPDEquipmentModificationWidget::HandleBoostLowClicked()
{
	UPDWidgetSoundLibrary::PlayUISound2D(this, ButtonClickSound);

	SetBoostType(EPDModificationBoostType::Low);
}

void UPDEquipmentModificationWidget::HandleBoostMidClicked()
{
	UPDWidgetSoundLibrary::PlayUISound2D(this, ButtonClickSound);

	SetBoostType(EPDModificationBoostType::Mid);
}

void UPDEquipmentModificationWidget::HandleBoostHighClicked()
{
	UPDWidgetSoundLibrary::PlayUISound2D(this, ButtonClickSound);

	SetBoostType(EPDModificationBoostType::High);
}

void UPDEquipmentModificationWidget::HandleModifyClicked()
{
	UPDWidgetSoundLibrary::PlayUISound2D(this, ButtonClickSound);

	if (!InventoryComponent || !ModificationComponent || SelectedSlotIndex == INDEX_NONE)
	{
		SetText(Text_Result, GetResultText(EPDModificationResult::InvalidSlot, false));
		return;
	}

	FPDModificationPreview Preview;
	EPDModificationResult Result = EPDModificationResult::InvalidSlot;
	const bool bSuccess = ModificationComponent->TryModifyInventorySlotWithMaterialAndBoostSlots(InventoryComponent, SelectedSlotIndex, SelectedMaterialSlotIndex, SelectedBoostSlotIndex, Preview, Result);
	SetText(Text_Result, GetResultText(Result, bSuccess));

	if (Preview.RequiredMaterials.Num() > 0 && (!InventoryComponent->Items.IsValidIndex(SelectedMaterialSlotIndex) || InventoryComponent->Items[SelectedMaterialSlotIndex].IsEmpty()))
	{
		const FName RequiredItemID = Preview.RequiredMaterials[0].RequiredItemID;
		SelectedMaterialSlotIndex = INDEX_NONE;
		for (int32 Index = 0; Index < InventoryComponent->Items.Num(); ++Index)
		{
			if (!InventoryComponent->Items[Index].IsEmpty() && InventoryComponent->Items[Index].ItemData.ItemID == RequiredItemID)
			{
				SelectedMaterialSlotIndex = Index;
				break;
			}
		}
	}

	RefreshAll();
}

void UPDEquipmentModificationWidget::HandleInventoryChanged()
{
	if (!IsValidEquipmentSlotIndex(SelectedSlotIndex))
	{
		SelectedSlotIndex = INDEX_NONE;
	}
	if (!IsValidMaterialSlotIndex(SelectedMaterialSlotIndex))
	{
		SelectedMaterialSlotIndex = INDEX_NONE;
	}
	if (!InventoryComponent || !InventoryComponent->Items.IsValidIndex(SelectedBoostSlotIndex) || InventoryComponent->Items[SelectedBoostSlotIndex].IsEmpty() || !IsRegisteredBoostItem(InventoryComponent->Items[SelectedBoostSlotIndex].ItemData.ItemID))
	{
		SelectedBoostSlotIndex = INDEX_NONE;
	}
	RefreshAll();
}

void UPDEquipmentModificationWidget::HandleModificationFinished(int32 InventorySlotIndex, int32 NewModificationLevel, bool bSuccess, EPDModificationResult Result)
{
	SetText(Text_Result, GetResultText(Result, bSuccess));
}

void UPDEquipmentModificationWidget::SetText(UTextBlock* TextBlock, const FText& Text) const
{
	if (TextBlock)
	{
		TextBlock->SetText(Text);
	}
}

void UPDEquipmentModificationWidget::SetSlotWidgetData(UPDInventorySlotWidget* SlotWidget, int32 SlotIndex, const FText& EmptyLabel) const
{
	if (!SlotWidget)
	{
		return;
	}

	SlotWidget->SetSlotContainerType(EPDItemContainerType::None);
	SlotWidget->SetEmptySlotLabel(EmptyLabel);

	if (InventoryComponent && InventoryComponent->Items.IsValidIndex(SlotIndex) && !InventoryComponent->Items[SlotIndex].IsEmpty())
	{
		FPDInventorySlot DisplaySlot = InventoryComponent->Items[SlotIndex];
		if (SlotWidget == MaterialSlotWidget && ModificationComponent && ModificationComponent->IsModificationMaterialItemID(DisplaySlot.ItemData.ItemID))
		{
			DisplaySlot.Quantity = CountItem(DisplaySlot.ItemData.ItemID);
		}
		SlotWidget->SetSlotData(DisplaySlot, SlotIndex);
		return;
	}

	SlotWidget->ClearSlotData(SlotIndex);
}

bool UPDEquipmentModificationWidget::IsValidEquipmentSlotIndex(int32 SlotIndex) const
{
	if (!InventoryComponent || !InventoryComponent->Items.IsValidIndex(SlotIndex) || InventoryComponent->Items[SlotIndex].IsEmpty())
	{
		return false;
	}

	const FPDItemData& ItemData = InventoryComponent->Items[SlotIndex].ItemData;
	return ItemData.ItemType == EPDItemType::Equipment && (ItemData.EquipmentSlotType == EPDEquipmentSlotType::Weapon || ItemData.EquipmentSlotType == EPDEquipmentSlotType::Armor || ItemData.EquipmentSlotType == EPDEquipmentSlotType::Head || ItemData.WeaponType != EWeaponType::None);
}

bool UPDEquipmentModificationWidget::IsValidMaterialSlotIndex(int32 SlotIndex) const
{
	if (!InventoryComponent || !ModificationComponent || !InventoryComponent->Items.IsValidIndex(SlotIndex) || InventoryComponent->Items[SlotIndex].IsEmpty())
	{
		return false;
	}

	return ModificationComponent->IsModificationMaterialItemID(InventoryComponent->Items[SlotIndex].ItemData.ItemID);
}

bool UPDEquipmentModificationWidget::IsRegisteredBoostItem(FName ItemID) const
{
	return ModificationComponent && ModificationComponent->IsRegisteredBoostItemID(ItemID);
}

bool UPDEquipmentModificationWidget::DoesSelectedMaterialMatchPreview(const FPDModificationPreview& Preview) const
{
	if (Preview.RequiredMaterials.Num() <= 0)
	{
		return true;
	}

	if (!InventoryComponent || !InventoryComponent->Items.IsValidIndex(SelectedMaterialSlotIndex))
	{
		return false;
	}

	const FPDInventorySlot& MaterialSlot = InventoryComponent->Items[SelectedMaterialSlotIndex];
	if (MaterialSlot.IsEmpty())
	{
		return false;
	}

	for (const FPDModificationMaterialRequirement& Material : Preview.RequiredMaterials)
	{
		if (MaterialSlot.ItemData.ItemID == Material.RequiredItemID && CountItem(Material.RequiredItemID) >= Material.Quantity)
		{
			return true;
		}
	}

	return false;
}

int32 UPDEquipmentModificationWidget::CountItem(FName ItemID) const
{
	if (!InventoryComponent || ItemID.IsNone())
	{
		return 0;
	}

	int32 Count = 0;
	for (const FPDInventorySlot& InventorySlotData : InventoryComponent->Items)
	{
		if (!InventorySlotData.IsEmpty() && InventorySlotData.ItemData.ItemID == ItemID)
		{
			Count += InventorySlotData.Quantity;
		}
	}
	return Count;
}

FText UPDEquipmentModificationWidget::GetResultText(EPDModificationResult Result, bool bSuccess) const
{
	if (Result == EPDModificationResult::Success && bSuccess)
	{
		return FText::FromString(TEXT("Modification success."));
	}

	switch (Result)
	{
	case EPDModificationResult::FailedByChance:
		return FText::FromString(TEXT("Modification failed."));
	case EPDModificationResult::InvalidInventory:
		return FText::FromString(TEXT("Inventory error."));
	case EPDModificationResult::InvalidSlot:
		return FText::FromString(TEXT("Select equipment."));
	case EPDModificationResult::NotEquipment:
		return FText::FromString(TEXT("Only equipment can be modified."));
	case EPDModificationResult::AlreadyMaxLevel:
		return FText::FromString(TEXT("Already max level."));
	case EPDModificationResult::MissingCurveTable:
		return FText::FromString(TEXT("Modification data is missing."));
	case EPDModificationResult::NotEnoughGold:
		return FText::FromString(TEXT("Not enough gold."));
	case EPDModificationResult::NotEnoughMaterials:
		return FText::FromString(TEXT("Not enough materials."));
	default:
		return FText::FromString(TEXT("Modification failed."));
	}
}

FText UPDEquipmentModificationWidget::FormatPercent(float Rate) const
{
	return FText::FromString(FString::Printf(TEXT("%.0f%%"), FMath::Clamp(Rate, 0.f, 1.f) * 100.f));
}

FText UPDEquipmentModificationWidget::FormatLevelText(int32 Level) const
{
	return FText::FromString(FString::Printf(TEXT("+%d"), FMath::Max(0, Level)));
}

FText UPDEquipmentModificationWidget::FormatGoldAmount(int32 Gold) const
{
	FNumberFormattingOptions Options;
	Options.UseGrouping = true;
	Options.MinimumIntegralDigits = 1;
	return FText::AsNumber(FMath::Max(0, Gold), &Options);
}

