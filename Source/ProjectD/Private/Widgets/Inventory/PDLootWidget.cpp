#include "Widgets/Inventory/PDLootWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/SizeBox.h"
#include "Components/SizeBoxSlot.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "GameFramework/Pawn.h"
#include "Items/PDInventoryComponent.h"
#include "Items/PDLootComponent.h"
#include "Widgets/Inventory/PDInventorySlotWidget.h"

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
	const int32 Rows    = FMath::Max(1, TargetLootComponent ? TargetLootComponent->GridRows    : FallbackGridRows);
	const int32 SlotCount = Columns * Rows;

	for (int32 SlotIndex = 0; SlotIndex < SlotCount; ++SlotIndex)
	{
		UUserWidget* CreatedSlotWidget = CreateWidget<UUserWidget>(GetOwningPlayer(), LootSlotWidgetClass);
		if (!CreatedSlotWidget) continue;

		if (UPDInventorySlotWidget* LootSlot = Cast<UPDInventorySlotWidget>(CreatedSlotWidget))
		{
			LootSlot->SetSlotContainerType(EPDItemContainerType::Loot);
			LootSlot->OnSlotLeftClicked.AddUniqueDynamic(this, &UPDLootWidget::HandleLootSlotLeftClicked);

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

		// Stash 위젯과 동일하게 SizeBox 로 셀 크기 고정 — UniformGrid 가 부모 사이즈에 늘어나지 않도록.
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

	APawn* OwnerPawn = GetOwningPlayerPawn();
	UPDInventoryComponent* Inventory = OwnerPawn ? OwnerPawn->FindComponentByClass<UPDInventoryComponent>() : nullptr;
	if (!Inventory) return;

	// 단발 정책: 좌클릭 1회 = 슬롯 전체 인벤토리로 이동.
	// 향후 드래그앤드롭/수량 팝업 등 확장 시 HandleLootSlotLeftClicked 분기 또는 별도 핸들러 추가.
	TargetLootComponent->TakeSlotToInventory(ClickedSlotIndex, Inventory, /*Quantity=*/-1);
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
