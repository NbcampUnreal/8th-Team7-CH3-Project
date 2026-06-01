#include "Widgets/Inventory/PDSecureContainerWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/SizeBox.h"
#include "Components/SizeBoxSlot.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/Widget.h"
#include "Core/PDPlayerComponentResolver.h"
#include "Core/PDPlayerController.h"
#include "GameFramework/Pawn.h"
#include "Items/Containers/PDInventoryComponent.h"
#include "Items/Containers/PDSecureContainerComponent.h"
#include "Widgets/Inventory/PDInventorySlotWidget.h"

void UPDSecureContainerWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BindSecureContainerChanged();
	RefreshSecureContainer();
}

void UPDSecureContainerWidget::NativeDestruct()
{
	UnbindSecureContainerChanged();

	Super::NativeDestruct();
}

void UPDSecureContainerWidget::RefreshSecureContainer()
{
	ResolveSecureGridPanel();

	if (!SecureGridPanel || !InventorySlotWidgetClass)
	{
		return;
	}

	UPDSecureContainerComponent* SecureContainerComponent = FindSecureContainerComponent();
	if (SecureContainerComponent)
	{
		SecureContainerComponent->InitializeSecureContainer();
	}

	const int32 SlotCount = SecureContainerComponent ? SecureContainerComponent->GetSlotCount() : FMath::Max(1, FallbackSlotCount);
	const int32 SafeColumns = FMath::Max(1, GridColumns);

	SecureGridPanel->ClearChildren();
	SecureGridPanel->SetSlotPadding(FMargin(0.f));
	SecureGridPanel->SetMinDesiredSlotWidth(SlotWidth);
	SecureGridPanel->SetMinDesiredSlotHeight(SlotHeight);

	for (int32 SlotIndex = 0; SlotIndex < SlotCount; ++SlotIndex)
	{
		UPDInventorySlotWidget* InventorySlotWidget = CreateWidget<UPDInventorySlotWidget>(GetOwningPlayer(), InventorySlotWidgetClass);
		if (!InventorySlotWidget)
		{
			continue;
		}

		InventorySlotWidget->SetSlotContainerType(EPDItemContainerType::SecureContainer);
		InventorySlotWidget->OnSlotItemDropped.AddUniqueDynamic(this, &UPDSecureContainerWidget::HandleSecureSlotItemDropped);

		const FPDInventorySlot* SecureSlot = SecureContainerComponent ? SecureContainerComponent->GetSecureSlot(SlotIndex) : nullptr;
		if (SecureSlot && !SecureSlot->IsEmpty())
		{
			InventorySlotWidget->SetSlotData(*SecureSlot, SlotIndex);
		}
		else
		{
			InventorySlotWidget->ClearSlotData(SlotIndex);
		}

		USizeBox* SlotSizeBox = WidgetTree ? WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass()) : nullptr;
		if (SlotSizeBox)
		{
			SlotSizeBox->SetWidthOverride(SlotWidth);
			SlotSizeBox->SetHeightOverride(SlotHeight);
			SlotSizeBox->SetMinDesiredWidth(SlotWidth);
			SlotSizeBox->SetMinDesiredHeight(SlotHeight);

			if (USizeBoxSlot* SizeBoxSlot = Cast<USizeBoxSlot>(SlotSizeBox->AddChild(InventorySlotWidget)))
			{
				SizeBoxSlot->SetHorizontalAlignment(HAlign_Fill);
				SizeBoxSlot->SetVerticalAlignment(VAlign_Fill);
				SizeBoxSlot->SetPadding(FMargin(0.f));
			}
		}

		UWidget* GridChildWidget = SlotSizeBox ? static_cast<UWidget*>(SlotSizeBox) : static_cast<UWidget*>(InventorySlotWidget);
		if (UUniformGridSlot* GridSlot = SecureGridPanel->AddChildToUniformGrid(GridChildWidget, SlotIndex / SafeColumns, SlotIndex % SafeColumns))
		{
			GridSlot->SetHorizontalAlignment(HAlign_Left);
			GridSlot->SetVerticalAlignment(VAlign_Top);
		}
	}
}

void UPDSecureContainerWidget::HandleSecureSlotItemDropped(UPDInventorySlotWidget* SlotWidget, int32 TargetSlotIndex, UPDInventoryDragDropOperation* DragOperation)
{
	if (!SlotWidget || !DragOperation || !DragOperation->IsValidPayload() || TargetSlotIndex == INDEX_NONE)
	{
		return;
	}

	UPDSecureContainerComponent* SecureContainerComponent = FindSecureContainerComponent();
	if (!SecureContainerComponent)
	{
		return;
	}

	switch (DragOperation->SourceContainerType)
	{
	case EPDItemContainerType::Inventory:
		if (UPDInventoryComponent* InventoryComponent = FindInventoryComponent())
		{
			SecureContainerComponent->StoreInventorySlotQuantityToSlot(InventoryComponent, DragOperation->SourceSlotIndex, TargetSlotIndex, DragOperation->SlotData.Quantity);
		}
		break;
	case EPDItemContainerType::SecureContainer:
		SecureContainerComponent->MoveSlotQuantityToSlot(DragOperation->SourceSlotIndex, TargetSlotIndex, DragOperation->SlotData.Quantity);
		break;
	default:
		break;
	}
}

void UPDSecureContainerWidget::ResolveSecureGridPanel()
{
	if (SecureGridPanel)
	{
		return;
	}

	if (!WidgetTree)
	{
		return;
	}

	if (!SecureGridWidgetName.IsNone())
	{
		SecureGridPanel = Cast<UUniformGridPanel>(WidgetTree->FindWidget(SecureGridWidgetName));
	}

	if (SecureGridPanel)
	{
		return;
	}

	WidgetTree->ForEachWidget([this](UWidget* Widget)
	{
		if (!SecureGridPanel)
		{
			SecureGridPanel = Cast<UUniformGridPanel>(Widget);
		}
	});
}

void UPDSecureContainerWidget::BindSecureContainerChanged()
{
	UPDSecureContainerComponent* SecureContainerComponent = FindSecureContainerComponent();
	if (BoundSecureContainerComponent == SecureContainerComponent)
	{
		return;
	}

	UnbindSecureContainerChanged();

	BoundSecureContainerComponent = SecureContainerComponent;
	if (BoundSecureContainerComponent)
	{
		BoundSecureContainerComponent->OnSecureContainerChanged.AddUniqueDynamic(this, &UPDSecureContainerWidget::RefreshSecureContainer);
	}
}

void UPDSecureContainerWidget::UnbindSecureContainerChanged()
{
	if (BoundSecureContainerComponent)
	{
		BoundSecureContainerComponent->OnSecureContainerChanged.RemoveDynamic(this, &UPDSecureContainerWidget::RefreshSecureContainer);
		BoundSecureContainerComponent = nullptr;
	}
}

UPDInventoryComponent* UPDSecureContainerWidget::FindInventoryComponent() const
{
	if (UPDInventoryComponent* Inventory = FPDPlayerComponentResolver::ResolveInventory(GetOwningPlayer()))
	{
		return Inventory;
	}
	return FPDPlayerComponentResolver::ResolveInventory(GetOwningPlayerPawn());
}

UPDSecureContainerComponent* UPDSecureContainerWidget::FindSecureContainerComponent() const
{
	if (APawn* OwningPawn = GetOwningPlayerPawn())
	{
		return OwningPawn->FindComponentByClass<UPDSecureContainerComponent>();
	}

	return nullptr;
}
