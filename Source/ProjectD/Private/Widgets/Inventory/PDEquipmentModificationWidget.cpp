#include "Widgets/Inventory/PDEquipmentModificationWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "GameFramework/Pawn.h"
#include "Items/PDEquipmentModificationComponent.h"
#include "Items/PDInventoryComponent.h"
#include "Widgets/Inventory/PDEquipmentListItemWidget.h"

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
	InventoryComponent = OwningPawn ? OwningPawn->FindComponentByClass<UPDInventoryComponent>() : nullptr;
	ModificationComponent = OwningPawn ? OwningPawn->FindComponentByClass<UPDEquipmentModificationComponent>() : nullptr;
}

void UPDEquipmentModificationWidget::BindWidgetEvents()
{
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
}

void UPDEquipmentModificationWidget::UnbindWidgetEvents()
{
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
	RefreshPreview();
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
		if (InventorySlotData.IsEmpty() || InventorySlotData.ItemData.ItemType != EPDItemType::Equipment)
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

void UPDEquipmentModificationWidget::SelectFirstEquipmentSlotIfNeeded()
{
	if (SelectedSlotIndex != INDEX_NONE || !InventoryComponent)
	{
		return;
	}

	for (int32 Index = 0; Index < InventoryComponent->Items.Num(); ++Index)
	{
		const FPDInventorySlot& InventorySlotData = InventoryComponent->Items[Index];
		if (!InventorySlotData.IsEmpty() && InventorySlotData.ItemData.ItemType == EPDItemType::Equipment)
		{
			SelectedSlotIndex = Index;
			return;
		}
	}
}

void UPDEquipmentModificationWidget::SelectInventorySlot(int32 SlotIndex)
{
	SelectedSlotIndex = SlotIndex;
	SetText(Text_Result, ModifyReadyText);
	RefreshPreview();
}

void UPDEquipmentModificationWidget::SetBoostType(EPDModificationBoostType BoostType)
{
	SelectedBoostType = BoostType;
	SetText(Text_Result, ModifyReadyText);
	RefreshPreview();
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

	bCurrentPreviewValid = ModificationComponent->CanModifyInventorySlotWithBoost(InventoryComponent, SelectedSlotIndex, SelectedBoostType, CurrentPreview, CurrentPreviewResult);
	if (!bCurrentPreviewValid && CurrentPreviewResult == EPDModificationResult::AlreadyMaxLevel)
	{
		ModificationComponent->GetModificationPreviewWithBoost(InventorySlotData, SelectedBoostType, CurrentPreview, CurrentPreviewResult);
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
		SetText(Text_StatType, FText::FromString(TEXT("공격력")));
		SetText(Text_StatPreview, FText::FromString(FString::Printf(TEXT("+%.1f"), Preview.AttackBonus)));
	}
	else if (Preview.DefenseBonus > 0.f)
	{
		SetText(Text_StatType, FText::FromString(TEXT("방어력")));
		SetText(Text_StatPreview, FText::FromString(FString::Printf(TEXT("+%.1f"), Preview.DefenseBonus)));
	}
	else
	{
		SetText(Text_StatType, FText::FromString(TEXT("-")));
		SetText(Text_StatPreview, FText::FromString(TEXT("-")));
	}

	SetText(Text_GoldCost, FText::FromString(FString::Printf(TEXT("%d / %d"), InventoryComponent ? InventoryComponent->GetGold() : 0, Preview.GoldCost)));
	SetText(Text_BaseSuccessRate, FormatPercent(Preview.BaseSuccessRate));
	SetText(Text_BoostSuccessRate, FText::FromString(FString::Printf(TEXT("+%s"), *FormatPercent(Preview.BoostSuccessRate).ToString())));
	SetText(Text_FinalSuccessRate, FormatPercent(Preview.SuccessRate));
	RefreshMaterialList(Preview);

	if (Button_Modify)
	{
		Button_Modify->SetIsEnabled(bCurrentPreviewValid);
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

void UPDEquipmentModificationWidget::HandleEquipmentListItemClicked(UPDEquipmentListItemWidget* ItemWidget, int32 SlotIndex)
{
	SelectInventorySlot(SlotIndex);
	RefreshEquipmentList();
}

void UPDEquipmentModificationWidget::HandleBoostNoneClicked()
{
	SetBoostType(EPDModificationBoostType::None);
}

void UPDEquipmentModificationWidget::HandleBoostLowClicked()
{
	SetBoostType(EPDModificationBoostType::Low);
}

void UPDEquipmentModificationWidget::HandleBoostMidClicked()
{
	SetBoostType(EPDModificationBoostType::Mid);
}

void UPDEquipmentModificationWidget::HandleBoostHighClicked()
{
	SetBoostType(EPDModificationBoostType::High);
}

void UPDEquipmentModificationWidget::HandleModifyClicked()
{
	if (!InventoryComponent || !ModificationComponent || SelectedSlotIndex == INDEX_NONE)
	{
		SetText(Text_Result, GetResultText(EPDModificationResult::InvalidSlot, false));
		return;
	}

	FPDModificationPreview Preview;
	EPDModificationResult Result = EPDModificationResult::InvalidSlot;
	const bool bSuccess = ModificationComponent->TryModifyInventorySlotWithBoost(InventoryComponent, SelectedSlotIndex, SelectedBoostType, Preview, Result);
	SetText(Text_Result, GetResultText(Result, bSuccess));
	RefreshEquipmentList();
	RefreshPreview();
}

void UPDEquipmentModificationWidget::HandleInventoryChanged()
{
	if (!InventoryComponent || !InventoryComponent->Items.IsValidIndex(SelectedSlotIndex) || InventoryComponent->Items[SelectedSlotIndex].IsEmpty())
	{
		SelectedSlotIndex = INDEX_NONE;
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
		return FText::FromString(TEXT("개조 성공"));
	}

	switch (Result)
	{
	case EPDModificationResult::FailedByChance:
		return FText::FromString(TEXT("개조 실패"));
	case EPDModificationResult::InvalidInventory:
		return FText::FromString(TEXT("인벤토리 오류"));
	case EPDModificationResult::InvalidSlot:
		return FText::FromString(TEXT("장비를 선택하세요"));
	case EPDModificationResult::NotEquipment:
		return FText::FromString(TEXT("장비만 개조할 수 있습니다"));
	case EPDModificationResult::AlreadyMaxLevel:
		return FText::FromString(TEXT("최대 개조 단계입니다"));
	case EPDModificationResult::MissingCurveTable:
		return FText::FromString(TEXT("개조 데이터가 없습니다"));
	case EPDModificationResult::NotEnoughGold:
		return FText::FromString(TEXT("골드가 부족합니다"));
	case EPDModificationResult::NotEnoughMaterials:
		return FText::FromString(TEXT("재료가 부족합니다"));
	default:
		return FText::FromString(TEXT("개조 실패"));
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
