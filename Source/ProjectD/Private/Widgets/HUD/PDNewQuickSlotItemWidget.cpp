#include "Widgets/HUD/PDNewQuickSlotItemWidget.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Animation/WidgetAnimation.h"
#include "Materials/MaterialInterface.h"
#include "Core/PDPlayerComponentResolver.h"
#include "Core/PDPlayerController.h"
#include "GameFramework/Pawn.h"
#include "Items/Containers/PDInventoryComponent.h"
#include "Items/Containers/PDQuickSlotComponent.h"
#include "Items/Containers/PDStashComponent.h"

void UPDNewQuickSlotItemWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	BuildFallbackWidget();
	if (WidgetTree)
	{
		if (UWidget* HotkeyWidget = WidgetTree->FindWidget(TEXT("Text_Hotkey")))
		{
			HotkeyWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	if (SlotBackground)
	{
		SlotBackground->SetVisibility(ESlateVisibility::Hidden);
	}
	SetSelected(false);
	ClearWeaponCooldownUI();
	RefreshVisuals();
}

void UPDNewQuickSlotItemWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildFallbackWidget();
	if (WidgetTree)
	{
		if (UWidget* HotkeyWidget = WidgetTree->FindWidget(TEXT("Text_Hotkey")))
		{
			HotkeyWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	if (SlotBackground)
	{
		SlotBackground->SetVisibility(ESlateVisibility::Hidden);
	}
	SetSlotSize(SlotSize);
	SetSelected(false);
	ClearWeaponCooldownUI();
	RefreshVisuals();
}

void UPDNewQuickSlotItemWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	SetSlotSize(SlotSize);
	SetSelected(bSelected);
}

void UPDNewQuickSlotItemWidget::InitializeQuickSlot(UPDQuickSlotComponent* InQuickSlotComponent, int32 InSlotIndex)
{
	QuickSlotComponent = InQuickSlotComponent;
	SlotIndex = InSlotIndex;
}

void UPDNewQuickSlotItemWidget::SetSlotData(const FPDInventorySlot& InSlotData)
{
	SlotData = InSlotData;
	RefreshVisuals();
}

void UPDNewQuickSlotItemWidget::ClearSlotData()
{
	SlotData.Clear();
	RefreshVisuals();
}

FReply UPDNewQuickSlotItemWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && !SlotData.IsEmpty())
	{
		return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton).NativeReply;
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UPDNewQuickSlotItemWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);

	if (SlotData.IsEmpty() || SlotIndex == INDEX_NONE)
	{
		return;
	}

	UPDInventoryDragDropOperation* DragOperation = NewObject<UPDInventoryDragDropOperation>(this);
	if (!DragOperation)
	{
		return;
	}

	DragOperation->SourceContainerType = EPDItemContainerType::QuickSlot;
	DragOperation->SourceSlotIndex = SlotIndex;
	DragOperation->SlotData = SlotData;
	DragOperation->Pivot = EDragPivot::MouseDown;
	OutOperation = DragOperation;
}

bool UPDNewQuickSlotItemWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	UPDInventoryDragDropOperation* DragOperation = Cast<UPDInventoryDragDropOperation>(InOperation);
	if (!DragOperation || !DragOperation->IsValidPayload() || !QuickSlotComponent || SlotIndex == INDEX_NONE)
	{
		return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
	}

	const int32 Quantity = FMath::Max(1, DragOperation->SlotData.Quantity);
	bool bMoved = false;

	switch (DragOperation->SourceContainerType)
	{
	case EPDItemContainerType::Inventory:
		if (APDPlayerController* PlayerController = Cast<APDPlayerController>(GetOwningPlayer()))
		{
			const AActor* QuickSlotOwner = QuickSlotComponent->GetOwner();
			if (!QuickSlotOwner || !QuickSlotOwner->HasAuthority())
			{
				PlayerController->ServerStoreInventorySlotQuantityToQuickSlot(DragOperation->SourceSlotIndex, SlotIndex, Quantity);
				return true;
			}
		}
		bMoved = QuickSlotComponent->StoreInventorySlotQuantityToSlot(FindInventoryComponent(), DragOperation->SourceSlotIndex, SlotIndex, Quantity);
		break;
	case EPDItemContainerType::Stash:
		return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
	case EPDItemContainerType::QuickSlot:
		if (APDPlayerController* PlayerController = Cast<APDPlayerController>(GetOwningPlayer()))
		{
			const AActor* QuickSlotOwner = QuickSlotComponent->GetOwner();
			if (!QuickSlotOwner || !QuickSlotOwner->HasAuthority())
			{
				PlayerController->ServerMoveQuickSlotQuantity(DragOperation->SourceSlotIndex, SlotIndex, Quantity);
				return true;
			}
		}
		bMoved = QuickSlotComponent->MoveSlotQuantityToSlot(DragOperation->SourceSlotIndex, SlotIndex, Quantity);
		break;
	default:
		break;
	}

	return bMoved || Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
}

void UPDNewQuickSlotItemWidget::BuildFallbackWidget()
{
	if (!WidgetTree || GetRootWidget())
	{
		return;
	}

	USizeBox* RootSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RootSizeBox"));
	RootSizeBox->SetWidthOverride(SlotSize.X);
	RootSizeBox->SetHeightOverride(SlotSize.Y);
	WidgetTree->RootWidget = RootSizeBox;

	UOverlay* Overlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("RootOverlay"));
	RootSizeBox->AddChild(Overlay);

	SlotBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SlotBackground"));
	SlotBackground->SetBrushColor(FLinearColor(0.08f, 0.08f, 0.08f, 0.7f));
	if (UOverlaySlot* BorderSlot = Overlay->AddChildToOverlay(SlotBackground))
	{
		BorderSlot->SetHorizontalAlignment(HAlign_Fill);
		BorderSlot->SetVerticalAlignment(VAlign_Fill);
	}

	Image_ItemIcon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Image_ItemIcon"));
	Image_ItemIcon->SetVisibility(ESlateVisibility::Hidden);
	if (UOverlaySlot* ImageSlot = Overlay->AddChildToOverlay(Image_ItemIcon))
	{
		ImageSlot->SetPadding(FMargin(6.f));
		ImageSlot->SetHorizontalAlignment(HAlign_Fill);
		ImageSlot->SetVerticalAlignment(VAlign_Fill);
	}

	Image_CooldownOverlay = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Image_CooldownOverlay"));
	Image_CooldownOverlay->SetColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.55f));
	Image_CooldownOverlay->SetVisibility(ESlateVisibility::Hidden);
	if (UOverlaySlot* CooldownOverlaySlot = Overlay->AddChildToOverlay(Image_CooldownOverlay))
	{
		CooldownOverlaySlot->SetHorizontalAlignment(HAlign_Fill);
		CooldownOverlaySlot->SetVerticalAlignment(VAlign_Fill);
	}

	Text_CooldownRemain = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Text_CooldownRemain"));
	Text_CooldownRemain->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	Text_CooldownRemain->SetShadowOffset(FVector2D(1.f, 1.f));
	Text_CooldownRemain->SetShadowColorAndOpacity(FLinearColor::Black);
	Text_CooldownRemain->SetJustification(ETextJustify::Center);
	Text_CooldownRemain->SetVisibility(ESlateVisibility::Hidden);
	if (UOverlaySlot* CooldownTextSlot = Overlay->AddChildToOverlay(Text_CooldownRemain))
	{
		CooldownTextSlot->SetHorizontalAlignment(HAlign_Center);
		CooldownTextSlot->SetVerticalAlignment(VAlign_Center);
	}

	Image_Flash = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Image_Flash"));
	Image_Flash->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.f));
	Image_Flash->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	if (UOverlaySlot* FlashSlot = Overlay->AddChildToOverlay(Image_Flash))
	{
		FlashSlot->SetHorizontalAlignment(HAlign_Fill);
		FlashSlot->SetVerticalAlignment(VAlign_Fill);
	}

	Text_Quantity = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Text_Quantity"));
	Text_Quantity->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	Text_Quantity->SetShadowOffset(FVector2D(1.f, 1.f));
	Text_Quantity->SetShadowColorAndOpacity(FLinearColor::Black);
	Text_Quantity->SetJustification(ETextJustify::Right);
	if (UOverlaySlot* QuantitySlot = Overlay->AddChildToOverlay(Text_Quantity))
	{
		QuantitySlot->SetPadding(FMargin(0.f, 0.f, 6.f, 4.f));
		QuantitySlot->SetHorizontalAlignment(HAlign_Right);
		QuantitySlot->SetVerticalAlignment(VAlign_Bottom);
	}
}

void UPDNewQuickSlotItemWidget::RefreshVisuals()
{
	BuildFallbackWidget();

	if (SlotData.IsEmpty())
	{
		if (Image_ItemIcon)
		{
			Image_ItemIcon->SetBrushFromTexture(nullptr);
			Image_ItemIcon->SetVisibility(ESlateVisibility::Hidden);
		}

		if (Text_Quantity)
		{
			Text_Quantity->SetText(FText::GetEmpty());
		}

		if (Text_AmmoOrCount)
		{
			Text_AmmoOrCount->SetText(FText::GetEmpty());
		}
		return;
	}

	if (Image_ItemIcon)
	{
		Image_ItemIcon->SetBrushFromTexture(SlotData.ItemData.Icon);
		Image_ItemIcon->SetVisibility(SlotData.ItemData.Icon ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}

	if (Text_Quantity)
	{
		Text_Quantity->SetText(SlotData.Quantity > 1 ? FText::AsNumber(SlotData.Quantity) : FText::GetEmpty());
	}

	if (Text_AmmoOrCount)
	{
		Text_AmmoOrCount->SetText(
			SlotData.Quantity > 1
				? FText::FromString(FString::Printf(TEXT("x%d"), SlotData.Quantity))
				: FText::GetEmpty());
	}
}

void UPDNewQuickSlotItemWidget::SetWeaponCooldownUI(bool bActive, float RemainingTime)
{
	const bool bVisible = bActive && RemainingTime > 0.f;

	if (Image_CooldownOverlay)
	{
		Image_CooldownOverlay->SetVisibility(bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Hidden);
	}

	if (Text_CooldownRemain)
	{
		Text_CooldownRemain->SetVisibility(bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Hidden);
		Text_CooldownRemain->SetText(bVisible ? FText::AsNumber(FMath::CeilToInt(RemainingTime)) : FText::GetEmpty());
	}
}

void UPDNewQuickSlotItemWidget::ClearWeaponCooldownUI()
{
	SetWeaponCooldownUI(false, 0.f);
}

void UPDNewQuickSlotItemWidget::PlayCooldownReadyFlash()
{
	if (!Image_Flash)
	{
		return;
	}

	if (Anim_CooldownReadyFlash)
	{
		StopAnimation(Anim_CooldownReadyFlash);
		PlayAnimation(Anim_CooldownReadyFlash);
		return;
	}

	Image_Flash->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.f));
}

UPDInventoryComponent* UPDNewQuickSlotItemWidget::FindInventoryComponent() const
{
	if (UPDInventoryComponent* Inventory = FPDPlayerComponentResolver::ResolveInventory(GetOwningPlayer()))
	{
		return Inventory;
	}
	return FPDPlayerComponentResolver::ResolveInventory(GetOwningPlayerPawn());
}

UPDStashComponent* UPDNewQuickSlotItemWidget::FindStashComponent() const
{
	if (APDPlayerController* PC = Cast<APDPlayerController>(GetOwningPlayer()))
	{
		return PC->GetActiveStashComponent();
	}
	return nullptr;
}

void UPDNewQuickSlotItemWidget::SetSelected(bool bNewSelected)
{
	bSelected = bNewSelected;

	if (Image_SlotBG)
	{
		const TSoftObjectPtr<UMaterialInterface>& TargetMat = bSelected ? SlotBGMaterial_Selected : SlotBGMaterial_Base;
		if (UMaterialInterface* Mat = TargetMat.LoadSynchronous())
		{
			Image_SlotBG->SetBrushFromMaterial(Mat);
		}
	}

	if (Container_AmmoLabel)
	{
		Container_AmmoLabel->SetVisibility(bSelected ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UPDNewQuickSlotItemWidget::SetKeyBindingIcon(UTexture2D* InIcon)
{
	if (Image_KeyBinding)
	{
		Image_KeyBinding->SetBrushFromTexture(InIcon);
		Image_KeyBinding->SetVisibility(InIcon ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
}

void UPDNewQuickSlotItemWidget::SetCountText(const FText& InCountText)
{
	if (Text_AmmoOrCount)
	{
		Text_AmmoOrCount->SetText(InCountText);
	}
}

void UPDNewQuickSlotItemWidget::SetSlotSize(FVector2D NewSize)
{
	SlotSize = NewSize;
	if (Box_Slot)
	{
		Box_Slot->SetWidthOverride(NewSize.X);
		Box_Slot->SetHeightOverride(NewSize.Y);
	}
}

void UPDNewQuickSlotItemWidget::SetSlotMaterials(TSoftObjectPtr<UMaterialInterface> InBase, TSoftObjectPtr<UMaterialInterface> InSelected)
{
	SlotBGMaterial_Base = InBase;
	SlotBGMaterial_Selected = InSelected;

	if (Image_SlotBG)
	{
		const TSoftObjectPtr<UMaterialInterface>& TargetMat = bSelected ? SlotBGMaterial_Selected : SlotBGMaterial_Base;
		if (UMaterialInterface* Mat = TargetMat.LoadSynchronous())
		{
			Image_SlotBG->SetBrushFromMaterial(Mat);
		}
	}
}
