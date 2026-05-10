#include "Widgets/Inventory/PDStashWidget.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "GameFramework/Pawn.h"
#include "Items/PDInventoryComponent.h"
#include "Items/PDStashComponent.h"
#include "Widgets/Inventory/PDInventorySlotWidget.h"
#include "Widgets/Inventory/PDQuantityPopupWidget.h"

void UPDStashWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BindStashChanged();
	RefreshStashGrid();
}

void UPDStashWidget::NativeDestruct()
{
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

	if (!StashSlotWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("PDStashWidget: StashSlotWidgetClass is not set. Set it to WBP_InventorySlot in WBP_Stash Class Defaults."));
		return;
	}

	UPDStashComponent* StashComponent = FindStashComponent();

	const int32 Columns = StashComponent ? FMath::Max(1, StashComponent->GridColumns) : FMath::Max(1, FallbackGridColumns);
	const int32 Rows = StashComponent ? FMath::Max(1, StashComponent->GridRows) : FMath::Max(1, FallbackGridRows);
	const int32 SlotCount = StashComponent ? StashComponent->GetMaxSlotCount() : Columns * Rows;

	for (int32 Index = 0; Index < SlotCount; ++Index)
	{
		UUserWidget* CreatedSlotWidget = CreateWidget<UUserWidget>(GetOwningPlayer(), StashSlotWidgetClass);
		if (!CreatedSlotWidget)
		{
			continue;
		}

		if (UPDInventorySlotWidget* StashSlotWidget = Cast<UPDInventorySlotWidget>(CreatedSlotWidget))
		{
			StashSlotWidget->OnSlotLeftClicked.AddUniqueDynamic(this, &UPDStashWidget::HandleStashSlotLeftClicked);

			if (StashComponent && StashComponent->StashItems.IsValidIndex(Index))
			{
				StashSlotWidget->SetSlotData(StashComponent->StashItems[Index], Index);
			}
			else
			{
				StashSlotWidget->ClearSlotData(Index);
			}
		}

		UUniformGridSlot* GridSlot = StashGridPanel->AddChildToUniformGrid(CreatedSlotWidget, Index / Columns, Index % Columns);
		if (GridSlot)
		{
			GridSlot->SetHorizontalAlignment(HAlign_Center);
			GridSlot->SetVerticalAlignment(VAlign_Center);
		}
	}
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

UPDStashComponent* UPDStashWidget::FindStashComponent() const
{
	if (APawn* OwningPawn = GetOwningPlayerPawn())
	{
		return OwningPawn->FindComponentByClass<UPDStashComponent>();
	}

	return nullptr;
}

UPDInventoryComponent* UPDStashWidget::FindInventoryComponent() const
{
	if (APawn* OwningPawn = GetOwningPlayerPawn())
	{
		return OwningPawn->FindComponentByClass<UPDInventoryComponent>();
	}

	return nullptr;
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

void UPDStashWidget::TakeStashSlotQuantity(int32 SlotIndex, int32 Quantity)
{
	UPDInventoryComponent* InventoryComponent = FindInventoryComponent();
	UPDStashComponent* StashComponent = FindStashComponent();

	if (!InventoryComponent || !StashComponent || Quantity <= 0)
	{
		return;
	}

	StashComponent->TakeStashSlotQuantity(InventoryComponent, SlotIndex, Quantity);
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

	PendingSlotIndex = SlotIndex;
	ActiveQuantityPopup->OnConfirmed.RemoveDynamic(this, &UPDStashWidget::HandleQuantityConfirmed);
	ActiveQuantityPopup->OnConfirmed.AddUniqueDynamic(this, &UPDStashWidget::HandleQuantityConfirmed);
	ActiveQuantityPopup->AddToViewport(100);
	ActiveQuantityPopup->InitializeQuantityPopup(MaxQuantity, Title);
}

void UPDStashWidget::HandleQuantityConfirmed(int32 Quantity)
{
	if (PendingSlotIndex != INDEX_NONE)
	{
		TakeStashSlotQuantity(PendingSlotIndex, Quantity);
	}

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
