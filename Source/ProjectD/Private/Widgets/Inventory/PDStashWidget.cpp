#include "Widgets/Inventory/PDStashWidget.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "GameFramework/Pawn.h"
#include "Items/PDInventoryComponent.h"
#include "Items/PDItemSlotTransfer.h"
#include "Items/PDQuickSlotComponent.h"
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
			StashSlotWidget->SetSlotContainerType(EPDItemContainerType::Stash);
			StashSlotWidget->OnSlotLeftClicked.AddUniqueDynamic(this, &UPDStashWidget::HandleStashSlotLeftClicked);
			StashSlotWidget->OnSlotItemDropped.AddUniqueDynamic(this, &UPDStashWidget::HandleStashSlotItemDropped);

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
	if (!SlotWidget || !DragOperation || !DragOperation->IsValidPayload())
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
			StashComponent->StoreInventorySlotQuantityToSlot(InventoryComponent, SourceSlotIndex, TargetSlotIndex, Quantity);
		}
		break;
	case EPDItemContainerType::Stash:
		StashComponent->MoveSlotQuantityToSlot(SourceSlotIndex, TargetSlotIndex, Quantity);
		break;
	case EPDItemContainerType::QuickSlot:
		if (UPDQuickSlotComponent* QuickSlotComponent = FindQuickSlotComponent())
		{
			QuickSlotComponent->TakeQuickSlotQuantityToStashSlot(StashComponent, SourceSlotIndex, TargetSlotIndex, Quantity);
		}
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
	ActiveQuantityPopup->AddToViewport(100);
	ActiveQuantityPopup->InitializeQuantityPopup(MaxQuantity, Title);
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
