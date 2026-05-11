#include "Widgets/Inventory/PDQuickSlotWidget.h"

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

void UPDQuickSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BindQuickSlotsChanged();
	RefreshQuickSlotGrid();
}

void UPDQuickSlotWidget::NativeDestruct()
{
	if (ActiveQuantityPopup && ActiveQuantityPopup->IsInViewport())
	{
		ActiveQuantityPopup->RemoveFromParent();
	}
	ClearQuantityRequest();
	UnbindQuickSlotsChanged();
	Super::NativeDestruct();
}

void UPDQuickSlotWidget::RefreshQuickSlotGrid()
{
	ResolveQuickSlotGridPanel();

	if (!QuickSlotGridPanel)
	{
		UE_LOG(LogTemp, Warning, TEXT("PDQuickSlotWidget: Quick slot grid widget was not found. Expected widget name: %s"), *QuickSlotGridWidgetName.ToString());
		return;
	}

	QuickSlotGridPanel->ClearChildren();

	if (!QuickSlotWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("PDQuickSlotWidget: QuickSlotWidgetClass is not set. Set it to WBP_InventorySlot in WBP_QuickSlot Class Defaults."));
		return;
	}

	UPDQuickSlotComponent* QuickSlotComponent = FindQuickSlotComponent();

	const int32 Columns = QuickSlotComponent ? FMath::Max(1, QuickSlotComponent->GridColumns) : FMath::Max(1, FallbackGridColumns);
	const int32 Rows = QuickSlotComponent ? FMath::Max(1, QuickSlotComponent->GridRows) : FMath::Max(1, FallbackGridRows);
	const int32 SlotCount = QuickSlotComponent ? QuickSlotComponent->GetMaxSlotCount() : Columns * Rows;

	for (int32 Index = 0; Index < SlotCount; ++Index)
	{
		UUserWidget* CreatedSlotWidget = CreateWidget<UUserWidget>(GetOwningPlayer(), QuickSlotWidgetClass);
		if (!CreatedSlotWidget)
		{
			continue;
		}

		if (UPDInventorySlotWidget* QuickSlotWidget = Cast<UPDInventorySlotWidget>(CreatedSlotWidget))
		{
			QuickSlotWidget->SetSlotContainerType(EPDItemContainerType::QuickSlot);
			QuickSlotWidget->OnSlotItemDropped.AddUniqueDynamic(this, &UPDQuickSlotWidget::HandleQuickSlotItemDropped);

			if (QuickSlotComponent && QuickSlotComponent->QuickSlotItems.IsValidIndex(Index))
			{
				QuickSlotWidget->SetSlotData(QuickSlotComponent->QuickSlotItems[Index], Index);
			}
			else
			{
				QuickSlotWidget->ClearSlotData(Index);
			}
		}

		UUniformGridSlot* GridSlot = QuickSlotGridPanel->AddChildToUniformGrid(CreatedSlotWidget, Index / Columns, Index % Columns);
		if (GridSlot)
		{
			GridSlot->SetHorizontalAlignment(HAlign_Center);
			GridSlot->SetVerticalAlignment(VAlign_Center);
		}
	}
}

void UPDQuickSlotWidget::ResolveQuickSlotGridPanel()
{
	if (QuickSlotGridPanel)
	{
		return;
	}

	if (!WidgetTree)
	{
		return;
	}

	if (UWidget* FoundWidget = WidgetTree->FindWidget(QuickSlotGridWidgetName))
	{
		QuickSlotGridPanel = Cast<UUniformGridPanel>(FoundWidget);
	}

	if (!QuickSlotGridPanel)
	{
		WidgetTree->ForEachWidget([this](UWidget* Widget)
		{
			if (!QuickSlotGridPanel)
			{
				QuickSlotGridPanel = Cast<UUniformGridPanel>(Widget);
			}
		});
	}
}

UPDInventoryComponent* UPDQuickSlotWidget::FindInventoryComponent() const
{
	if (APawn* OwningPawn = GetOwningPlayerPawn())
	{
		return OwningPawn->FindComponentByClass<UPDInventoryComponent>();
	}

	return nullptr;
}

UPDStashComponent* UPDQuickSlotWidget::FindStashComponent() const
{
	if (APawn* OwningPawn = GetOwningPlayerPawn())
	{
		return OwningPawn->FindComponentByClass<UPDStashComponent>();
	}

	return nullptr;
}

UPDQuickSlotComponent* UPDQuickSlotWidget::FindQuickSlotComponent() const
{
	if (APawn* OwningPawn = GetOwningPlayerPawn())
	{
		return OwningPawn->FindComponentByClass<UPDQuickSlotComponent>();
	}

	return nullptr;
}

const FPDInventorySlot* UPDQuickSlotWidget::FindQuickSlot(int32 SlotIndex) const
{
	const UPDQuickSlotComponent* QuickSlotComponent = FindQuickSlotComponent();
	return QuickSlotComponent && QuickSlotComponent->QuickSlotItems.IsValidIndex(SlotIndex) ? &QuickSlotComponent->QuickSlotItems[SlotIndex] : nullptr;
}

const FPDInventorySlot* UPDQuickSlotWidget::FindSourceSlot(EPDItemContainerType SourceContainerType, int32 SlotIndex) const
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
		if (const UPDStashComponent* StashComponent = FindStashComponent())
		{
			return StashComponent->StashItems.IsValidIndex(SlotIndex) ? &StashComponent->StashItems[SlotIndex] : nullptr;
		}
		return nullptr;
	case EPDItemContainerType::QuickSlot:
		return FindQuickSlot(SlotIndex);
	default:
		return nullptr;
	}
}

void UPDQuickSlotWidget::HandleQuickSlotItemDropped(UPDInventorySlotWidget* SlotWidget, int32 TargetSlotIndex, UPDInventoryDragDropOperation* DragOperation)
{
	if (!SlotWidget || !DragOperation || !DragOperation->IsValidPayload())
	{
		return;
	}

	if (DragOperation->SourceContainerType == EPDItemContainerType::QuickSlot)
	{
		ExecuteQuickSlotTransfer(DragOperation->SourceContainerType, DragOperation->SourceSlotIndex, TargetSlotIndex, DragOperation->SlotData.Quantity);
		return;
	}

	int32 MaxQuantity = 0;
	FText Title;
	if (ShouldOpenTransferQuantityPopup(DragOperation->SourceContainerType, DragOperation->SourceSlotIndex, TargetSlotIndex, MaxQuantity, Title))
	{
		OpenTransferQuantityPopup(DragOperation->SourceContainerType, DragOperation->SourceSlotIndex, TargetSlotIndex, MaxQuantity, Title);
		return;
	}

	ExecuteQuickSlotTransfer(DragOperation->SourceContainerType, DragOperation->SourceSlotIndex, TargetSlotIndex, DragOperation->SlotData.Quantity);
}

void UPDQuickSlotWidget::ExecuteQuickSlotTransfer(EPDItemContainerType SourceContainerType, int32 SourceSlotIndex, int32 TargetSlotIndex, int32 Quantity)
{
	if (Quantity <= 0)
	{
		return;
	}

	UPDQuickSlotComponent* QuickSlotComponent = FindQuickSlotComponent();
	if (!QuickSlotComponent)
	{
		return;
	}

	switch (SourceContainerType)
	{
	case EPDItemContainerType::Inventory:
		if (UPDInventoryComponent* InventoryComponent = FindInventoryComponent())
		{
			QuickSlotComponent->StoreInventorySlotQuantityToSlot(InventoryComponent, SourceSlotIndex, TargetSlotIndex, Quantity);
		}
		break;
	case EPDItemContainerType::Stash:
		if (UPDStashComponent* StashComponent = FindStashComponent())
		{
			QuickSlotComponent->StoreStashSlotQuantityToSlot(StashComponent, SourceSlotIndex, TargetSlotIndex, Quantity);
		}
		break;
	case EPDItemContainerType::QuickSlot:
		QuickSlotComponent->MoveSlotQuantityToSlot(SourceSlotIndex, TargetSlotIndex, Quantity);
		break;
	default:
		break;
	}
}

bool UPDQuickSlotWidget::ShouldOpenTransferQuantityPopup(EPDItemContainerType SourceContainerType, int32 SourceSlotIndex, int32 TargetSlotIndex, int32& OutMaxQuantity, FText& OutTitle) const
{
	OutMaxQuantity = 0;
	OutTitle = FText::GetEmpty();

	if (SourceContainerType == EPDItemContainerType::QuickSlot)
	{
		return false;
	}

	const FPDInventorySlot* SourceSlot = FindSourceSlot(SourceContainerType, SourceSlotIndex);
	const FPDInventorySlot* TargetSlot = FindQuickSlot(TargetSlotIndex);
	if (!SourceSlot || !TargetSlot || SourceSlot->IsEmpty())
	{
		return false;
	}

	OutMaxQuantity = FPDItemSlotTransfer::GetQuantityLimit(*SourceSlot, *TargetSlot);
	OutTitle = SourceSlot->ItemData.DisplayName.IsEmpty() ? FText::FromName(SourceSlot->ItemData.ItemID) : SourceSlot->ItemData.DisplayName;
	return FPDItemSlotTransfer::CanPromptForPartial(*SourceSlot, *TargetSlot) && OutMaxQuantity > 1;
}

void UPDQuickSlotWidget::OpenTransferQuantityPopup(EPDItemContainerType SourceContainerType, int32 SourceSlotIndex, int32 TargetSlotIndex, int32 MaxQuantity, const FText& Title)
{
	if (!QuantityPopupWidgetClass)
	{
		ExecuteQuickSlotTransfer(SourceContainerType, SourceSlotIndex, TargetSlotIndex, MaxQuantity);
		return;
	}

	if (ActiveQuantityPopup && ActiveQuantityPopup->IsInViewport())
	{
		ActiveQuantityPopup->RemoveFromParent();
	}

	ActiveQuantityPopup = CreateWidget<UPDQuantityPopupWidget>(GetOwningPlayer(), QuantityPopupWidgetClass);
	if (!ActiveQuantityPopup)
	{
		ExecuteQuickSlotTransfer(SourceContainerType, SourceSlotIndex, TargetSlotIndex, MaxQuantity);
		return;
	}

	PendingTransferSourceContainerType = SourceContainerType;
	PendingTransferSourceSlotIndex = SourceSlotIndex;
	PendingTransferTargetSlotIndex = TargetSlotIndex;
	ActiveQuantityPopup->OnConfirmed.RemoveDynamic(this, &UPDQuickSlotWidget::HandleQuantityConfirmed);
	ActiveQuantityPopup->OnConfirmed.AddUniqueDynamic(this, &UPDQuickSlotWidget::HandleQuantityConfirmed);
	ActiveQuantityPopup->OnCancelled.RemoveDynamic(this, &UPDQuickSlotWidget::HandleQuantityCancelled);
	ActiveQuantityPopup->OnCancelled.AddUniqueDynamic(this, &UPDQuickSlotWidget::HandleQuantityCancelled);
	ActiveQuantityPopup->AddToViewport(100);
	ActiveQuantityPopup->InitializeQuantityPopup(MaxQuantity, Title);
}

void UPDQuickSlotWidget::HandleQuantityConfirmed(int32 Quantity)
{
	ExecuteQuickSlotTransfer(PendingTransferSourceContainerType, PendingTransferSourceSlotIndex, PendingTransferTargetSlotIndex, Quantity);
	ClearQuantityRequest();
}

void UPDQuickSlotWidget::HandleQuantityCancelled()
{
	ClearQuantityRequest();
}

void UPDQuickSlotWidget::ClearQuantityRequest()
{
	PendingTransferSourceContainerType = EPDItemContainerType::None;
	PendingTransferSourceSlotIndex = INDEX_NONE;
	PendingTransferTargetSlotIndex = INDEX_NONE;
	ActiveQuantityPopup = nullptr;
}

void UPDQuickSlotWidget::BindQuickSlotsChanged()
{
	UPDQuickSlotComponent* QuickSlotComponent = FindQuickSlotComponent();

	if (BoundQuickSlotComponent == QuickSlotComponent)
	{
		return;
	}

	UnbindQuickSlotsChanged();

	BoundQuickSlotComponent = QuickSlotComponent;

	if (BoundQuickSlotComponent)
	{
		BoundQuickSlotComponent->OnQuickSlotsChanged.AddUniqueDynamic(this, &UPDQuickSlotWidget::RefreshQuickSlotGrid);
	}
}

void UPDQuickSlotWidget::UnbindQuickSlotsChanged()
{
	if (BoundQuickSlotComponent)
	{
		BoundQuickSlotComponent->OnQuickSlotsChanged.RemoveDynamic(this, &UPDQuickSlotWidget::RefreshQuickSlotGrid);
		BoundQuickSlotComponent = nullptr;
	}
}
