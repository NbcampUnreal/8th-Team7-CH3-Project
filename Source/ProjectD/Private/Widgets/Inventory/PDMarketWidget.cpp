#include "Widgets/Inventory/PDMarketWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Core/PDPlayerComponentResolver.h"
#include "Core/PDPlayerController.h"
#include "GameFramework/Pawn.h"
#include "Items/Containers/PDInventoryComponent.h"
#include "Items/Market/PDMarketComponent.h"
#include "Widgets/Inventory/PDMarketItemWidget.h"

void UPDMarketWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BindMarketChanged();
	BindInventoryChanged();
	RefreshMarketInfo();
	RefreshMarketGoods();
}

void UPDMarketWidget::NativeDestruct()
{
	UnbindInventoryChanged();
	UnbindMarketChanged();
	Super::NativeDestruct();
}

void UPDMarketWidget::InitializeMarket(UPDMarketComponent* InMarketComponent)
{
	MarketComponent = InMarketComponent;
	BindMarketChanged();
	BindInventoryChanged();
	RefreshMarketInfo();
	RefreshMarketGoods();
}

void UPDMarketWidget::RefreshMarketInfo()
{
	ResolveMarketInfoTextBlocks();
	RefreshInventoryGold();
}

void UPDMarketWidget::RefreshMarketGoods()
{
	ResolveMarketListPanel();
	RefreshInventoryGold();

	if (MarketListPanel)
	{
		MarketListPanel->ClearChildren();
	}

	if (!MarketComponent || !MarketListPanel || !MarketRowWidgetClass)
	{
		return;
	}

	UPDInventoryComponent* BuyerInventory = FindInventoryComponent();

	for (int32 EntryIndex = 0; EntryIndex < MarketComponent->Goods.Num(); ++EntryIndex)
	{
		if (!MarketComponent->ShouldShowEntry(EntryIndex))
		{
			continue;
		}

		UPDMarketItemWidget* RowWidget = CreateWidget<UPDMarketItemWidget>(GetOwningPlayer(), MarketRowWidgetClass);
		if (!RowWidget)
		{
			continue;
		}

		RowWidget->SetInventorySlotWidgetClass(MarketSlotWidgetClass);
		RowWidget->SetQuantityPopupWidgetClass(MarketQuantityPopupWidgetClass);
		RowWidget->SetMarketEntry(MarketComponent, BuyerInventory, MarketComponent->Goods[EntryIndex], EntryIndex);
		MarketListPanel->AddChild(RowWidget);
	}
}

void UPDMarketWidget::ResolveMarketInfoTextBlocks()
{
	if (!WidgetTree)
	{
		return;
	}

	if (!TextInventoryGold && !InventoryGoldTextWidgetName.IsNone())
	{
		TextInventoryGold = Cast<UTextBlock>(WidgetTree->FindWidget(InventoryGoldTextWidgetName));
	}
}

void UPDMarketWidget::ResolveMarketListPanel()
{
	if (MarketListPanel || !WidgetTree || MarketListWidgetName.IsNone())
	{
		return;
	}

	MarketListPanel = Cast<UPanelWidget>(WidgetTree->FindWidget(MarketListWidgetName));
}

void UPDMarketWidget::BindMarketChanged()
{
	if (BoundMarketComponent == MarketComponent)
	{
		return;
	}

	UnbindMarketChanged();
	BoundMarketComponent = MarketComponent;

	if (BoundMarketComponent)
	{
		BoundMarketComponent->OnMarketChanged.AddUniqueDynamic(this, &UPDMarketWidget::RefreshMarketGoods);
	}
}

void UPDMarketWidget::UnbindMarketChanged()
{
	if (BoundMarketComponent)
	{
		BoundMarketComponent->OnMarketChanged.RemoveDynamic(this, &UPDMarketWidget::RefreshMarketGoods);
		BoundMarketComponent = nullptr;
	}
}

void UPDMarketWidget::BindInventoryChanged()
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
		BoundInventoryComponent->OnInventoryChanged.AddUniqueDynamic(this, &UPDMarketWidget::RefreshMarketInfo);
		BoundInventoryComponent->OnInventoryChanged.AddUniqueDynamic(this, &UPDMarketWidget::RefreshMarketGoods);
	}
}

void UPDMarketWidget::UnbindInventoryChanged()
{
	if (BoundInventoryComponent)
	{
		BoundInventoryComponent->OnInventoryChanged.RemoveDynamic(this, &UPDMarketWidget::RefreshMarketInfo);
		BoundInventoryComponent->OnInventoryChanged.RemoveDynamic(this, &UPDMarketWidget::RefreshMarketGoods);
		BoundInventoryComponent = nullptr;
	}
}

void UPDMarketWidget::RefreshInventoryGold()
{
	ResolveMarketInfoTextBlocks();

	if (TextInventoryGold)
	{
		const UPDInventoryComponent* InventoryComponent = FindInventoryComponent();
		TextInventoryGold->SetText(MakeGoldText(InventoryComponent ? InventoryComponent->GetGold() : 0));
	}
}

FText UPDMarketWidget::MakeGoldText(int32 Gold) const
{
	return FText::AsNumber(FMath::Max(0, Gold));
}

UPDInventoryComponent* UPDMarketWidget::FindInventoryComponent() const
{
	if (UPDInventoryComponent* Inventory = FPDPlayerComponentResolver::ResolveInventory(GetOwningPlayer()))
	{
		return Inventory;
	}
	return FPDPlayerComponentResolver::ResolveInventory(GetOwningPlayerPawn());
}
