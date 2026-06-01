#include "Widgets/Inventory/PDLootWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/SizeBox.h"
#include "Components/SizeBoxSlot.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Core/PDPlayerComponentResolver.h"
#include "Core/PDPlayerController.h"
#include "GameFramework/Pawn.h"
#include "Items/Containers/PDInventoryComponent.h"
#include "Items/Containers/PDLootComponent.h"
#include "Widgets/Inventory/PDInventorySlotWidget.h"
#include "Widgets/Inventory/PDInventoryDragDropOperation.h"

void UPDLootWidget::InitializeLoot(UPDLootComponent* InLootComponent)
{
	if (TargetLootComponent == InLootComponent)
	{
		RefreshLootGrid();
		return;
	}

	UnbindLootChanged();
	TargetLootComponent = InLootComponent;
	BindLootChanged();
	RefreshLootGrid();
}

void UPDLootWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BindLootChanged();
	RefreshLootGrid();
}

void UPDLootWidget::NativeDestruct()
{
	UnbindLootChanged();
	Super::NativeDestruct();
}

void UPDLootWidget::ResolveGridPanel()
{
	if (LootGridPanel) return;
	if (!WidgetTree) return;

	if (UWidget* FoundWidget = WidgetTree->FindWidget(LootGridWidgetName))
	{
		LootGridPanel = Cast<UUniformGridPanel>(FoundWidget);
	}

	if (!LootGridPanel && bUseGridPanelFallback)
	{
		WidgetTree->ForEachWidget([this](UWidget* Widget)
		{
			if (!LootGridPanel)
			{
				LootGridPanel = Cast<UUniformGridPanel>(Widget);
			}
		});
	}
}

void UPDLootWidget::RefreshLootGrid()
{
	ResolveGridPanel();

	if (!LootGridPanel)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("PDLootWidget: Loot grid widget was not found. Expected widget name: %s"),
			*LootGridWidgetName.ToString());
		return;
	}

	if (!LootSlotWidgetClass)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("PDLootWidget: LootSlotWidgetClass is not set. Set it in WBP_Loot Class Defaults (PD|Loot category)."));
		return;
	}

	LootGridPanel->ClearChildren();
	LootGridPanel->SetSlotPadding(FMargin(0.f));
	LootGridPanel->SetMinDesiredSlotWidth(SlotWidth);
	LootGridPanel->SetMinDesiredSlotHeight(SlotHeight);

	const int32 Columns = FMath::Max(1, TargetLootComponent ? TargetLootComponent->GridColumns : FallbackGridColumns);
	// 슬롯 수는 컴포넌트의 실제 LootItems 크기(가변 행)를 따름 — 콘텐츠에 맞춰 행이 늘어남.
	const int32 SlotCount = TargetLootComponent
		? TargetLootComponent->LootItems.Num()
		: (FMath::Max(1, FallbackGridColumns) * FMath::Max(1, FallbackGridRows));

	for (int32 SlotIndex = 0; SlotIndex < SlotCount; ++SlotIndex)
	{
		UUserWidget* CreatedSlotWidget = CreateWidget<UUserWidget>(GetOwningPlayer(), LootSlotWidgetClass);
		if (!CreatedSlotWidget) continue;

		if (UPDInventorySlotWidget* LootSlot = Cast<UPDInventorySlotWidget>(CreatedSlotWidget))
		{
			LootSlot->SetSlotContainerType(EPDItemContainerType::Loot);
			LootSlot->OnSlotLeftClicked.AddUniqueDynamic(this, &UPDLootWidget::HandleLootSlotLeftClicked);
			LootSlot->OnSlotItemDropped.AddUniqueDynamic(this, &UPDLootWidget::HandleLootSlotItemDropped);

			if (TargetLootComponent && TargetLootComponent->LootItems.IsValidIndex(SlotIndex)
				&& !TargetLootComponent->LootItems[SlotIndex].IsEmpty())
			{
				LootSlot->SetSlotData(TargetLootComponent->LootItems[SlotIndex], SlotIndex);
			}
			else
			{
				LootSlot->ClearSlotData(SlotIndex);
			}
		}

		// Stash ?�젯�??�일?�게 SizeBox �??� ?�기 고정 ??UniformGrid 가 부�??�이즈에 ?�어?��? ?�도�?
		USizeBox* SlotSizeBox = WidgetTree ? WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass()) : nullptr;
		if (SlotSizeBox)
		{
			SlotSizeBox->SetWidthOverride(SlotWidth);
			SlotSizeBox->SetHeightOverride(SlotHeight);
			SlotSizeBox->SetMinDesiredWidth(SlotWidth);
			SlotSizeBox->SetMinDesiredHeight(SlotHeight);

			if (USizeBoxSlot* SizeBoxSlot = Cast<USizeBoxSlot>(SlotSizeBox->AddChild(CreatedSlotWidget)))
			{
				SizeBoxSlot->SetHorizontalAlignment(HAlign_Fill);
				SizeBoxSlot->SetVerticalAlignment(VAlign_Fill);
				SizeBoxSlot->SetPadding(FMargin(0.f));
			}
		}

		UWidget* GridChildWidget = SlotSizeBox ? static_cast<UWidget*>(SlotSizeBox) : static_cast<UWidget*>(CreatedSlotWidget);
		UUniformGridSlot* GridSlot = LootGridPanel->AddChildToUniformGrid(GridChildWidget, SlotIndex / Columns, SlotIndex % Columns);
		if (GridSlot)
		{
			GridSlot->SetHorizontalAlignment(HAlign_Left);
			GridSlot->SetVerticalAlignment(VAlign_Top);
		}
	}
}

void UPDLootWidget::HandleLootSlotLeftClicked(UPDInventorySlotWidget* /*SlotWidget*/, int32 ClickedSlotIndex)
{
	if (!TargetLootComponent) return;

	UPDInventoryComponent* Inventory = FPDPlayerComponentResolver::ResolveInventory(GetOwningPlayer());
	if (!Inventory)
	{
		Inventory = FPDPlayerComponentResolver::ResolveInventory(GetOwningPlayerPawn());
	}
	if (!Inventory) return;

	// ?�발 ?�책: 좌클�?1??= ?�롯 ?�체 ?�벤?�리�??�동.
	// ?�후 ?�래그앤?�롭/?�량 ?�업 ???�장 ??HandleLootSlotLeftClicked 분기 ?�는 별도 ?�들??추�?.
	TargetLootComponent->TakeSlotToInventory(ClickedSlotIndex, Inventory, /*Quantity=*/-1);
}

void UPDLootWidget::HandleLootSlotItemDropped(UPDInventorySlotWidget* /*SlotWidget*/, int32 TargetSlotIndex, UPDInventoryDragDropOperation* DragOperation)
{
	if (!TargetLootComponent || !DragOperation || !DragOperation->IsValidPayload() || TargetSlotIndex == INDEX_NONE)
	{
		return;
	}

	const int32 Quantity = DragOperation->SlotData.Quantity;

	switch (DragOperation->SourceContainerType)
	{
	case EPDItemContainerType::Loot:
		// 박스 내부 재배치/스왑.
		TargetLootComponent->MoveSlotQuantityToSlot(DragOperation->SourceSlotIndex, TargetSlotIndex, Quantity);
		break;

	case EPDItemContainerType::Inventory:
	{
		// 인벤토리 → 박스에 넣기.
		UPDInventoryComponent* Inventory = FPDPlayerComponentResolver::ResolveInventory(GetOwningPlayer());
		if (!Inventory)
		{
			Inventory = FPDPlayerComponentResolver::ResolveInventory(GetOwningPlayerPawn());
		}
		if (Inventory)
		{
			TargetLootComponent->StoreInventorySlotQuantityToSlot(Inventory, DragOperation->SourceSlotIndex, TargetSlotIndex, Quantity);
		}
		break;
	}

	default:
		// Stash/QuickSlot/Equipment 등은 박스로 직접 받지 않음.
		break;
	}
}

void UPDLootWidget::BindLootChanged()
{
	if (TargetLootComponent && BoundLootComponent != TargetLootComponent)
	{
		UnbindLootChanged();
		BoundLootComponent = TargetLootComponent;
		BoundLootComponent->OnLootChanged.AddUniqueDynamic(this, &UPDLootWidget::RefreshLootGrid);
	}
}

void UPDLootWidget::UnbindLootChanged()
{
	if (BoundLootComponent)
	{
		BoundLootComponent->OnLootChanged.RemoveDynamic(this, &UPDLootWidget::RefreshLootGrid);
		BoundLootComponent = nullptr;
	}
}
