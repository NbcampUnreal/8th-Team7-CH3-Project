
#include "Widgets/Inventory/PDInventoryWidget.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/TextBlock.h"
#include "Components/PanelWidget.h"
#include "Core/PDPlayerController.h"
#include "GameFramework/Pawn.h"
#include "Items/PDInventoryComponent.h"
#include "Items/PDStashComponent.h"
#include "Widgets/Inventory/PDInventoryItemContextMenuWidget.h"
#include "Widgets/Inventory/PDInventorySlotWidget.h"
#include "Widgets/Inventory/PDQuantityPopupWidget.h"

void UPDInventoryWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
}

void UPDInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BindInventoryChanged();
	RefreshInventoryGrid();
}

void UPDInventoryWidget::NativeDestruct()
{
	CloseContextMenu();
	CloseItemHoverTooltip();
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

	if (!InventorySlotWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("PDInventoryWidget: InventorySlotWidgetClass is not set. Set it to WBP_InventorySlot in WBP_Inventory Class Defaults."));
		return;
	}

	UPDInventoryComponent* InventoryComponent = FindInventoryComponent();

	const int32 Columns = InventoryComponent ? FMath::Max(1, InventoryComponent->GridColumns) : FMath::Max(1, FallbackGridColumns);
	const int32 Rows = InventoryComponent ? FMath::Max(1, InventoryComponent->GridRows) : FMath::Max(1, FallbackGridRows);
	const int32 SlotCount = InventoryComponent ? InventoryComponent->GetMaxSlotCount() : Columns * Rows;

	for (int32 Index = 0; Index < SlotCount; ++Index)
	{
		UUserWidget* CreatedSlotWidget = CreateWidget<UUserWidget>(GetOwningPlayer(), InventorySlotWidgetClass);
		if (!CreatedSlotWidget)
		{
			continue;
		}

		if (UPDInventorySlotWidget* InventorySlotWidget = Cast<UPDInventorySlotWidget>(CreatedSlotWidget))
		{
			InventorySlotWidget->OnSlotLeftClicked.AddUniqueDynamic(this, &UPDInventoryWidget::HandleInventorySlotLeftClicked);
			InventorySlotWidget->OnSlotRightClicked.AddUniqueDynamic(this, &UPDInventoryWidget::HandleInventorySlotRightClicked);
			InventorySlotWidget->OnSlotHovered.AddUniqueDynamic(this, &UPDInventoryWidget::HandleInventorySlotHovered);
			InventorySlotWidget->OnSlotUnhovered.AddUniqueDynamic(this, &UPDInventoryWidget::HandleInventorySlotUnhovered);

			if (InventoryComponent && InventoryComponent->Items.IsValidIndex(Index))
			{
				InventorySlotWidget->SetSlotData(InventoryComponent->Items[Index], Index);
			}
			else
			{
				InventorySlotWidget->ClearSlotData(Index);
			}
		}

		UUniformGridSlot* GridSlot = InventoryGridPanel->AddChildToUniformGrid(CreatedSlotWidget, Index / Columns, Index % Columns);
		if (GridSlot)
		{
			GridSlot->SetHorizontalAlignment(HAlign_Center);
			GridSlot->SetVerticalAlignment(VAlign_Center);
		}
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
	if (!GoldTextWidget && WidgetTree && !GoldTextWidgetName.IsNone())
	{
		GoldTextWidget = Cast<UTextBlock>(WidgetTree->FindWidget(GoldTextWidgetName));
	}

	if (!GoldTextWidget)
	{
		return;
	}

	const UPDInventoryComponent* InventoryComponent = FindInventoryComponent();
	GoldTextWidget->SetText(FText::AsNumber(InventoryComponent ? InventoryComponent->GetGold() : 0));
}

UPDInventoryComponent* UPDInventoryWidget::FindInventoryComponent() const
{
	if (APawn* OwningPawn = GetOwningPlayerPawn())
	{
		return OwningPawn->FindComponentByClass<UPDInventoryComponent>();
	}

	return nullptr;
}

UPDStashComponent* UPDInventoryWidget::FindStashComponent() const
{
	if (APawn* OwningPawn = GetOwningPlayerPawn())
	{
		return OwningPawn->FindComponentByClass<UPDStashComponent>();
	}

	return nullptr;
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

	if (!SlotWidget || !ContextMenuWidgetClass)
	{
		return;
	}

	const FPDInventorySlot& SlotData = SlotWidget->GetSlotData();
	if (SlotData.IsEmpty())
	{
		return;
	}

	OpenItemHoverTooltip(SlotWidget);

	UPanelWidget* ContextMenuContainer = FindContextMenuContainer();
	if (!ContextMenuContainer)
	{
		return;
	}

	ActiveContextMenu = CreateWidget<UPDInventoryItemContextMenuWidget>(GetOwningPlayer(), ContextMenuWidgetClass);
	if (!ActiveContextMenu)
	{
		return;
	}

	ActiveContextMenu->OnUseClicked.AddUniqueDynamic(this, &UPDInventoryWidget::HandleContextMenuUseClicked);
	ActiveContextMenu->OnDropClicked.AddUniqueDynamic(this, &UPDInventoryWidget::HandleContextMenuDropClicked);
	ActiveContextMenu->OnEquipClicked.AddUniqueDynamic(this, &UPDInventoryWidget::HandleContextMenuEquipClicked);
	ActiveContextMenu->InitializeContextMenu(SlotIndex, SlotData);
	ContextMenuContainer->ClearChildren();
	ContextMenuContainer->AddChild(ActiveContextMenu);
	ContextMenuContainer->SetVisibility(ESlateVisibility::Visible);
	ActiveContextMenu->ForceLayoutPrepass();
}

void UPDInventoryWidget::CloseContextMenu()
{
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
	CloseContextMenu();
	CloseItemHoverTooltip();

	if (UPDInventoryComponent* InventoryComponent = FindInventoryComponent())
	{
		InventoryComponent->UseItemFromSlot(SlotIndex);
	}
}

void UPDInventoryWidget::HandleContextMenuDropClicked(UPDInventoryItemContextMenuWidget* MenuWidget, int32 SlotIndex)
{
	CloseContextMenu();
	CloseItemHoverTooltip();

	if (UPDInventoryComponent* InventoryComponent = FindInventoryComponent())
	{
		InventoryComponent->DropItemFromSlot(SlotIndex, 1);
	}
}

void UPDInventoryWidget::HandleContextMenuEquipClicked(UPDInventoryItemContextMenuWidget* MenuWidget, int32 SlotIndex)
{
	CloseContextMenu();
	CloseItemHoverTooltip();
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

	PendingSlotIndex = SlotIndex;
	ActiveQuantityPopup->OnConfirmed.RemoveDynamic(this, &UPDInventoryWidget::HandleQuantityConfirmed);
	ActiveQuantityPopup->OnConfirmed.AddUniqueDynamic(this, &UPDInventoryWidget::HandleQuantityConfirmed);
	ActiveQuantityPopup->AddToViewport(100);
	ActiveQuantityPopup->InitializeQuantityPopup(MaxQuantity, Title);
}

void UPDInventoryWidget::HandleQuantityConfirmed(int32 Quantity)
{
	if (PendingSlotIndex != INDEX_NONE)
	{
		ExecuteInventoryQuickAction(PendingSlotIndex, Quantity);
	}

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
	}
}

void UPDInventoryWidget::UnbindInventoryChanged()
{
	if (BoundInventoryComponent)
	{
		BoundInventoryComponent->OnInventoryChanged.RemoveDynamic(this, &UPDInventoryWidget::RefreshInventoryGrid);
		BoundInventoryComponent = nullptr;
	}
}
