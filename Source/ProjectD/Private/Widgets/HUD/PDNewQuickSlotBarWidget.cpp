#include "Widgets/HUD/PDNewQuickSlotBarWidget.h"

#include "Data/PDKeyIconDataAsset.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Items/PDQuickSlotComponent.h"
#include "TimerManager.h"
#include "Widgets/HUD/PDNewQuickSlotItemWidget.h"

void UPDNewQuickSlotBarWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	BindQuickSlotComponent(FindQuickSlotComponent());
}

void UPDNewQuickSlotBarWidget::NativeConstruct()
{
	Super::NativeConstruct();
	CollectSlotWidgets();
	BindQuickSlotComponent(FindQuickSlotComponent());

	if (ULocalPlayer* LP = GetOwningLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Sub = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			Sub->ControlMappingsRebuiltDelegate.AddUniqueDynamic(this, &UPDNewQuickSlotBarWidget::HandleControlMappingsRebuilt);
		}
	}
}

void UPDNewQuickSlotBarWidget::NativeDestruct()
{
	StopWeaponCooldownUITimer();
	ClearWeaponCooldownUI();

	if (BoundQuickSlotComponent)
	{
		BoundQuickSlotComponent->OnQuickSlotsChanged.RemoveDynamic(this, &UPDNewQuickSlotBarWidget::HandleQuickSlotsChanged);
		BoundQuickSlotComponent->OnSelectionChanged.RemoveDynamic(this, &UPDNewQuickSlotBarWidget::HandleSelectionChanged);
		BoundQuickSlotComponent->OnWeaponQuickSlotCooldownFinished.RemoveDynamic(this, &UPDNewQuickSlotBarWidget::HandleWeaponCooldownFinished);
		BoundQuickSlotComponent->OnWeaponQuickSlotCooldownStarted.RemoveDynamic(this, &UPDNewQuickSlotBarWidget::HandleWeaponCooldownStarted);
		BoundQuickSlotComponent->OnConsumableUseCompleted.RemoveDynamic(this, &UPDNewQuickSlotBarWidget::HandleConsumableUseCompleted);
		BoundQuickSlotComponent->OnConsumableUseCanceled.RemoveDynamic(this, &UPDNewQuickSlotBarWidget::HandleConsumableUseCanceled);
		BoundQuickSlotComponent->OnConsumableUseStarted.RemoveDynamic(this, &UPDNewQuickSlotBarWidget::HandleConsumableUseStarted);
	}

	if (ULocalPlayer* LP = GetOwningLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Sub = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			Sub->ControlMappingsRebuiltDelegate.RemoveDynamic(this, &UPDNewQuickSlotBarWidget::HandleControlMappingsRebuilt);
		}
	}

	Super::NativeDestruct();
}

void UPDNewQuickSlotBarWidget::BindQuickSlotComponent(UPDQuickSlotComponent* InQuickSlotComponent)
{
	if (BoundQuickSlotComponent == InQuickSlotComponent)
	{
		RefreshSlots();
		return;
	}

	if (BoundQuickSlotComponent)
	{
		BoundQuickSlotComponent->OnQuickSlotsChanged.RemoveDynamic(this, &UPDNewQuickSlotBarWidget::HandleQuickSlotsChanged);
		BoundQuickSlotComponent->OnSelectionChanged.RemoveDynamic(this, &UPDNewQuickSlotBarWidget::HandleSelectionChanged);
		BoundQuickSlotComponent->OnConsumableUseStarted.RemoveDynamic(this, &UPDNewQuickSlotBarWidget::HandleConsumableUseStarted);
		BoundQuickSlotComponent->OnConsumableUseCanceled.RemoveDynamic(this, &UPDNewQuickSlotBarWidget::HandleConsumableUseCanceled);
		BoundQuickSlotComponent->OnConsumableUseCompleted.RemoveDynamic(this, &UPDNewQuickSlotBarWidget::HandleConsumableUseCompleted);
		BoundQuickSlotComponent->OnWeaponQuickSlotCooldownStarted.RemoveDynamic(this, &UPDNewQuickSlotBarWidget::HandleWeaponCooldownStarted);
		BoundQuickSlotComponent->OnWeaponQuickSlotCooldownFinished.RemoveDynamic(this, &UPDNewQuickSlotBarWidget::HandleWeaponCooldownFinished);
	}

	BoundQuickSlotComponent = InQuickSlotComponent;

	if (BoundQuickSlotComponent)
	{
		BoundQuickSlotComponent->GridColumns = 6;
		BoundQuickSlotComponent->GridRows = 1;
		BoundQuickSlotComponent->SetWeaponSlotCount(WeaponSlotCount);
		BoundQuickSlotComponent->InitializeQuickSlots();
		BoundQuickSlotComponent->OnQuickSlotsChanged.AddUniqueDynamic(this, &UPDNewQuickSlotBarWidget::HandleQuickSlotsChanged);
		BoundQuickSlotComponent->OnSelectionChanged.AddUniqueDynamic(this, &UPDNewQuickSlotBarWidget::HandleSelectionChanged);
		BoundQuickSlotComponent->OnConsumableUseStarted.AddUniqueDynamic(this, &UPDNewQuickSlotBarWidget::HandleConsumableUseStarted);
		BoundQuickSlotComponent->OnConsumableUseCanceled.AddUniqueDynamic(this, &UPDNewQuickSlotBarWidget::HandleConsumableUseCanceled);
		BoundQuickSlotComponent->OnConsumableUseCompleted.AddUniqueDynamic(this, &UPDNewQuickSlotBarWidget::HandleConsumableUseCompleted);
		BoundQuickSlotComponent->OnWeaponQuickSlotCooldownStarted.AddUniqueDynamic(this, &UPDNewQuickSlotBarWidget::HandleWeaponCooldownStarted);
		BoundQuickSlotComponent->OnWeaponQuickSlotCooldownFinished.AddUniqueDynamic(this, &UPDNewQuickSlotBarWidget::HandleWeaponCooldownFinished);
	}

	CollectSlotWidgets();
	RefreshSlots();
}

void UPDNewQuickSlotBarWidget::CollectSlotWidgets()
{
	SlotWidgets.Reset();
	SlotWidgets.Add(Slot_0);
	SlotWidgets.Add(Slot_1);
	SlotWidgets.Add(Slot_2);
	SlotWidgets.Add(Slot_3);
	SlotWidgets.Add(Slot_4);
	SlotWidgets.Add(Slot_5);

	ApplyKeyBindings();
}

void UPDNewQuickSlotBarWidget::RefreshSlots()
{
	if (SlotWidgets.Num() == 0)
	{
		CollectSlotWidgets();
	}

	for (int32 Index = 0; Index < SlotWidgets.Num(); ++Index)
	{
		UPDNewQuickSlotItemWidget* SlotWidget = SlotWidgets[Index];
		if (!SlotWidget)
		{
			continue;
		}

		SlotWidget->InitializeQuickSlot(BoundQuickSlotComponent, Index);

		if (BoundQuickSlotComponent && BoundQuickSlotComponent->QuickSlotItems.IsValidIndex(Index))
		{
			SlotWidget->SetSlotData(BoundQuickSlotComponent->QuickSlotItems[Index]);
		}
		else
		{
			SlotWidget->ClearSlotData();
		}
	}

	ApplySelection(BoundQuickSlotComponent ? BoundQuickSlotComponent->GetSelectedIndex() : INDEX_NONE);
	UpdateWeaponCooldownUI();
}

void UPDNewQuickSlotBarWidget::HandleQuickSlotsChanged()
{
	RefreshSlots();
}

void UPDNewQuickSlotBarWidget::HandleSelectionChanged(int32 NewIndex)
{
	ApplySelection(NewIndex);
}

void UPDNewQuickSlotBarWidget::HandleConsumableUseStarted(int32 SlotIndex, FPDItemData ItemData, float Duration)
{
	RefreshSlots();
}

void UPDNewQuickSlotBarWidget::HandleConsumableUseCanceled(int32 SlotIndex, FPDItemData ItemData)
{
	RefreshSlots();
}

void UPDNewQuickSlotBarWidget::HandleConsumableUseCompleted(int32 SlotIndex, FPDItemData ItemData)
{
	RefreshSlots();
}

void UPDNewQuickSlotBarWidget::HandleWeaponCooldownStarted(int32 SlotIndex, float Duration, float EndTime)
{
	if (!IsWeaponCooldownUISlot(SlotIndex))
	{
		return;
	}

	ActiveWeaponCooldownSlotIndex = SlotIndex;
	SetWeaponCooldownUIForSlot(SlotIndex, Duration);
	StartWeaponCooldownUITimer();
}

void UPDNewQuickSlotBarWidget::HandleWeaponCooldownFinished(int32 SlotIndex)
{
	if (IsWeaponCooldownUISlot(SlotIndex))
	{
		SetWeaponCooldownUIForSlot(SlotIndex, 0.f);

		if (SlotWidgets.IsValidIndex(SlotIndex) && SlotWidgets[SlotIndex])
		{
			SlotWidgets[SlotIndex]->PlayCooldownReadyFlash();
		}
	}

	if (ActiveWeaponCooldownSlotIndex == SlotIndex)
	{
		ActiveWeaponCooldownSlotIndex = INDEX_NONE;
	}

	StopWeaponCooldownUITimer();
}

void UPDNewQuickSlotBarWidget::ApplySelection(int32 SelectedIndex)
{
	for (int32 Index = 0; Index < SlotWidgets.Num(); ++Index)
	{
		if (UPDNewQuickSlotItemWidget* SlotWidget = SlotWidgets[Index])
		{
			SlotWidget->SetSelected(Index == SelectedIndex);
		}
	}
}

void UPDNewQuickSlotBarWidget::ApplyKeyBindings()
{
	UPDKeyIconDataAsset* IconMap = KeyIconMap.LoadSynchronous();

	for (int32 Index = 0; Index < SlotWidgets.Num(); ++Index)
	{
		UPDNewQuickSlotItemWidget* SlotWidget = SlotWidgets[Index];
		if (!SlotWidget)
		{
			continue;
		}

		const UInputAction* Action = SlotInputActions.IsValidIndex(Index) ? SlotInputActions[Index] : nullptr;
		const FKey Key = FindKeyForAction(Action);

		UTexture2D* Icon = (IconMap && Key.IsValid()) ? IconMap->ResolveIcon(Key) : nullptr;
		SlotWidget->SetKeyBindingIcon(Icon);
	}
}

void UPDNewQuickSlotBarWidget::StartWeaponCooldownUITimer()
{
	UWorld* World = GetWorld();
	if (!World || World->GetTimerManager().IsTimerActive(WeaponCooldownUITimerHandle))
	{
		return;
	}

	World->GetTimerManager().SetTimer(WeaponCooldownUITimerHandle, this, &UPDNewQuickSlotBarWidget::UpdateWeaponCooldownUI, 0.1f, true);
}

void UPDNewQuickSlotBarWidget::StopWeaponCooldownUITimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(WeaponCooldownUITimerHandle);
	}
}

void UPDNewQuickSlotBarWidget::UpdateWeaponCooldownUI()
{
	if (!BoundQuickSlotComponent || !BoundQuickSlotComponent->IsWeaponQuickSlotOnCooldown())
	{
		if (ActiveWeaponCooldownSlotIndex != INDEX_NONE)
		{
			SetWeaponCooldownUIForSlot(ActiveWeaponCooldownSlotIndex, 0.f);
			ActiveWeaponCooldownSlotIndex = INDEX_NONE;
		}

		StopWeaponCooldownUITimer();
		return;
	}

	const int32 CooldownSlotIndex = BoundQuickSlotComponent->GetWeaponQuickSlotCooldownSlotIndex();
	if (!IsWeaponCooldownUISlot(CooldownSlotIndex))
	{
		ClearWeaponCooldownUI();
		StopWeaponCooldownUITimer();
		return;
	}

	ActiveWeaponCooldownSlotIndex = CooldownSlotIndex;
	SetWeaponCooldownUIForSlot(CooldownSlotIndex, BoundQuickSlotComponent->GetWeaponQuickSlotCooldownRemainingTime());
	StartWeaponCooldownUITimer();
}

void UPDNewQuickSlotBarWidget::SetWeaponCooldownUIForSlot(int32 SlotIndex, float RemainingTime)
{
	for (int32 Index = 0; Index < SlotWidgets.Num(); ++Index)
	{
		UPDNewQuickSlotItemWidget* SlotWidget = SlotWidgets[Index];
		if (!SlotWidget || !IsWeaponCooldownUISlot(Index))
		{
			continue;
		}

		SlotWidget->SetWeaponCooldownUI(Index == SlotIndex && RemainingTime > 0.f, RemainingTime);
	}
}

void UPDNewQuickSlotBarWidget::ClearWeaponCooldownUI()
{
	for (int32 Index = 0; Index < SlotWidgets.Num(); ++Index)
	{
		if (SlotWidgets[Index] && IsWeaponCooldownUISlot(Index))
		{
			SlotWidgets[Index]->SetWeaponCooldownUI(false, 0.f);
		}
	}
}

bool UPDNewQuickSlotBarWidget::IsWeaponCooldownUISlot(int32 SlotIndex) const
{
	return SlotIndex >= 0 && SlotIndex < WeaponSlotCount;
}

FKey UPDNewQuickSlotBarWidget::FindKeyForAction(const UInputAction* Action) const
{
	if (!Action || !InputMappingContext)
	{
		return FKey();
	}

	for (const FEnhancedActionKeyMapping& Mapping : InputMappingContext->GetMappings())
	{
		if (Mapping.Action == Action)
		{
			return Mapping.Key;
		}
	}

	return FKey();
}

void UPDNewQuickSlotBarWidget::HandleControlMappingsRebuilt()
{
	ApplyKeyBindings();
}

UPDQuickSlotComponent* UPDNewQuickSlotBarWidget::FindQuickSlotComponent() const
{
	if (APawn* Pawn = GetOwningPlayerPawn())
	{
		return Pawn->FindComponentByClass<UPDQuickSlotComponent>();
	}
	return nullptr;
}