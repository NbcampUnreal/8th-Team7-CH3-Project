#include "Core/PDPlayerUIManagerComponent.h"

#include "AbilitySystemComponent.h"
#include "Blueprint/UserWidget.h"
#include "Characters/Base/PDCharacterBase.h"
#include "Characters/PDPlayerCharacter.h"
#include "Components/Widget.h"
#include "Core/PDPlayerController.h"
#include "Core/PDPlayerState.h"
#include "Data/PDQuestComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Items/Containers/PDInventoryComponent.h"
#include "Items/Market/PDMarketComponent.h"
#include "Items/Containers/PDStashComponent.h"
#include "Subsystems/PDFrontendUISubsystem.h"
#include "Weapons/Base/PDWeaponBase.h"
#include "Widgets/HUD/PDHUDWidget.h"
#include "Widgets/HUD/PDWorldMapWidget.h"
#include "Widgets/Inventory/PDInventoryWidget.h"
#include "Widgets/Inventory/PDMarketWidget.h"
#include "Widgets/Inventory/PDStashWidget.h"
#include "Widgets/PDActivatableBase.h"
#include "Widgets/PDNotificationWidget.h"
#include "Widgets/PDRootLayout.h"
#include "Widgets/Quest/PDQuestWindowWidget.h"
#include "Widgets/Screen/PDTabbedScreenBase.h"

namespace
{
	bool ShouldShowCrosshairForController(const APDPlayerController* PC)
	{
		// Show the default crosshair whenever a local PD player is active.
		// Weapon-specific code only changes the crosshair type.
		const APDPlayerCharacter* PlayerCharacter = PC ? Cast<APDPlayerCharacter>(PC->GetPawn()) : nullptr;
		return PlayerCharacter != nullptr;
	}
}

UPDPlayerUIManagerComponent::UPDPlayerUIManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

APDPlayerController* UPDPlayerUIManagerComponent::GetPDController() const
{
	return Cast<APDPlayerController>(GetOwner());
}

void UPDPlayerUIManagerComponent::ConfigureLegacyWidgetClasses(
	TSubclassOf<UPDHUDWidget> InHUDClass,
	TSubclassOf<UPDRootLayout> InRootLayoutClass,
	TSubclassOf<UPDInventoryWidget> InInventoryWidgetClass,
	TSubclassOf<UPDStashWidget> InStashWidgetClass,
	TSubclassOf<UPDMarketWidget> InMarketWidgetClass,
	TSubclassOf<UUserWidget> InEquipmentModificationWidgetClass,
	TSubclassOf<UPDNotificationWidget> InNotificationWidgetClass,
	float InNotificationDuration,
	int32 InNotificationZOrder,
	TSubclassOf<UPDQuestWindowWidget> InQuestWindowWidgetClass,
	TSubclassOf<UPDWorldMapWidget> InWorldMapClass)
{
	if (!HUDClass) HUDClass = InHUDClass;
	if (!RootLayoutClass) RootLayoutClass = InRootLayoutClass;
	if (!InventoryWidgetClass) InventoryWidgetClass = InInventoryWidgetClass;
	if (!StashWidgetClass) StashWidgetClass = InStashWidgetClass;
	if (!MarketWidgetClass) MarketWidgetClass = InMarketWidgetClass;
	if (!EquipmentModificationWidgetClass) EquipmentModificationWidgetClass = InEquipmentModificationWidgetClass;
	if (!NotificationWidgetClass) NotificationWidgetClass = InNotificationWidgetClass;
	if (!QuestWindowWidgetClass) QuestWindowWidgetClass = InQuestWindowWidgetClass;
	if (!WorldMapClass) WorldMapClass = InWorldMapClass;

	NotificationDuration = InNotificationDuration;
	NotificationZOrder = InNotificationZOrder;
}

void UPDPlayerUIManagerComponent::InitializeUI(APDPlayerController* InOwnerController)
{
	APDPlayerController* PC = InOwnerController ? InOwnerController : GetPDController();
	if (!PC || !PC->IsLocalController())
	{
		return;
	}

	CreateAndAddHUDWidget();

	if (RootLayoutClass && !RootLayoutInstance)
	{
		RootLayoutInstance = CreateWidget<UPDRootLayout>(PC, RootLayoutClass);
		if (RootLayoutInstance)
		{
			RootLayoutInstance->AddToViewport(RootLayoutViewportZOrder);
			if (UPDFrontendUISubsystem* UISubsystem = UPDFrontendUISubsystem::Get(PC))
			{
				UISubsystem->RegisterRootLayout(RootLayoutInstance);
				UISubsystem->OnEffectiveUIStateChanged.RemoveAll(this);
				UISubsystem->OnEffectiveUIStateChanged.AddUObject(this, &UPDPlayerUIManagerComponent::ApplyEffectiveUIState);
				ApplyEffectiveUIState(EWidgetInputMode::Game);
			}
		}
	}

	EnsureRaidParticipationBinding();
}

void UPDPlayerUIManagerComponent::ShutdownUI(const EEndPlayReason::Type EndPlayReason)
{
	APDPlayerController* PC = GetPDController();

	UnbindRaidParticipation();

	CloseQuest();
	CloseMarket();
	CloseStash();
	CloseEquipmentModification();

	if (NotificationWidgetInstance)
	{
		NotificationWidgetInstance->RemoveFromParent();
		NotificationWidgetInstance = nullptr;
	}

	if (HUDInstance)
	{
		HUDInstance->Deactivate();
		HUDInstance->RemoveFromParent();
		HUDInstance = nullptr;
	}

	if (WorldMapInstance)
	{
		WorldMapInstance->RemoveFromParent();
		WorldMapInstance = nullptr;
	}

	if (PC)
	{
		if (UPDFrontendUISubsystem* UISubsystem = UPDFrontendUISubsystem::Get(PC))
		{
			UISubsystem->OnEffectiveUIStateChanged.RemoveAll(this);
			UISubsystem->UnregisterRootLayout();
		}
	}

	if (RootLayoutInstance)
	{
		RootLayoutInstance->RemoveFromParent();
		RootLayoutInstance = nullptr;
	}
}

void UPDPlayerUIManagerComponent::CreateAndAddHUDWidget()
{
	APDPlayerController* PC = GetPDController();
	if (!PC || !PC->IsLocalController() || !HUDClass)
	{
		return;
	}

	if (!HUDInstance)
	{
		HUDInstance = CreateWidget<UPDHUDWidget>(PC, HUDClass);
	}

	AddWidgetToViewportIfNeeded(HUDInstance, HUDViewportZOrder);
	if (HUDInstance)
	{
		HUDInstance->Activate();
		HUDInstance->RefreshNewQuickSlots();
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
			{
				RefreshNewQuickSlots();
			}));
		}
	}
}

void UPDPlayerUIManagerComponent::RebindHUDToASC(UAbilitySystemComponent* ASC)
{
	if (HUDInstance)
	{
		HUDInstance->RebindToASC(ASC);
	}
}

void UPDPlayerUIManagerComponent::ApplyEffectiveUIState(EWidgetInputMode Mode)
{
	APDPlayerController* PC = GetPDController();
	if (!PC)
	{
		return;
	}

	// Stack??effective top 모드???�라 PC InputMode + 커서�??�괄 ?�용.
	// stack??비면 Subsystem??Game??broadcast ???�기??GameOnly�?복원 ??WASD/Tab ??PC ?�력 ?�상??
	switch (Mode)
	{
	case EWidgetInputMode::Game:
		{
			FInputModeGameOnly InputMode;
			PC->SetInputMode(InputMode);
			PC->SetShowMouseCursor(false);
			break;
		}
	case EWidgetInputMode::GameAndMenu:
		{
			FInputModeGameAndUI InputMode;
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			InputMode.SetHideCursorDuringCapture(false);
			PC->SetInputMode(InputMode);
			PC->SetShowMouseCursor(true);
			break;
		}
	case EWidgetInputMode::Menu:
		{
			FInputModeUIOnly InputMode;
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PC->SetInputMode(InputMode);
			PC->SetShowMouseCursor(true);
			break;
		}
	case EWidgetInputMode::Cinematic:
		{
			// 연출: 게임 입력 차단(UIOnly) + 커서 숨김. 트랜지션/컷신 전용.
			FInputModeUIOnly InputMode;
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PC->SetInputMode(InputMode);
			PC->SetShowMouseCursor(false);
			break;
		}
	case EWidgetInputMode::Passive:
		// 모드/커서 건드리�? ?�음 (top ?�젯??명시?�으�?비�????�언).
		break;
	}

	if (HUDInstance)
	{
		HUDInstance->SetCrosshairVisible(
			Mode == EWidgetInputMode::Game && ShouldShowCrosshairForController(PC));
	}
}

void UPDPlayerUIManagerComponent::AddWidgetToViewportIfNeeded(UUserWidget* Widget, int32 ZOrder) const
{
	if (Widget && !Widget->IsInViewport())
	{
		Widget->AddToViewport(ZOrder);
	}
}

void UPDPlayerUIManagerComponent::OpenMarket(UPDMarketComponent* MarketComponent)
{
	APDPlayerController* PC = GetPDController();
	if (!PC || !PC->IsLocalController()) return;
	if (!MarketComponent)
	{
		UE_LOG(LogPDCharacter, Warning, TEXT("MarketComponent is not valid."));
		return;
	}
	if (!MarketWidgetClass)
	{
		UE_LOG(LogPDCharacter, Warning, TEXT("Market UI widget class is not set."));
		return;
	}

	ActiveMarketComponent = MarketComponent;
	CloseStash();
	CloseEquipmentModification();

	if (!MarketWidgetInstance)
	{
		MarketWidgetInstance = CreateWidget<UPDMarketWidget>(PC, MarketWidgetClass);
	}
	if (MarketWidgetInstance)
	{
		MarketWidgetInstance->InitializeMarket(MarketComponent);
		AddWidgetToViewportIfNeeded(MarketWidgetInstance, GameplayOverlayZOrder);
	}

	SetGameplayInputBlockedByModalUI(true, MarketWidgetInstance);
}

void UPDPlayerUIManagerComponent::CloseMarket()
{
	UPDMarketComponent* ClosedMarketComponent = ActiveMarketComponent.Get();

	if (MarketWidgetInstance)
	{
		MarketWidgetInstance->RemoveFromParent();
		MarketWidgetInstance = nullptr;
	}
	ActiveMarketComponent = nullptr;

	if (InventoryWidgetInstance && !IsStashOpen() && !IsEquipmentModificationOpen())
	{
		InventoryWidgetInstance->RemoveFromParent();
		InventoryWidgetInstance = nullptr;
	}

	if (!IsStashOpen() && !IsQuestOpen() && !IsEquipmentModificationOpen())
	{
		SetGameplayInputBlockedByModalUI(false);
	}

	if (ClosedMarketComponent)
	{
		if (APDPlayerController* PC = GetPDController())
		{
			PC->OnMarketInterfaceClosed.Broadcast(ClosedMarketComponent);
		}
	}
}

bool UPDPlayerUIManagerComponent::IsMarketOpen() const
{
	return MarketWidgetInstance && MarketWidgetInstance->IsInViewport();
}

void UPDPlayerUIManagerComponent::OpenStash(UPDStashComponent* StashSource)
{
	APDPlayerController* PC = GetPDController();
	if (!PC || !PC->IsLocalController()) return;
	if (!InventoryWidgetClass || !StashWidgetClass)
	{
		UE_LOG(LogPDCharacter, Warning, TEXT("Stash UI widget class is not set."));
		return;
	}
	if (!StashSource)
	{
		UE_LOG(LogPDCharacter, Warning, TEXT("OpenStash called with null StashSource."));
		return;
	}

	CloseMarket();
	CloseEquipmentModification();
	ActiveStashComponent = StashSource;

	if (!InventoryWidgetInstance)
	{
		InventoryWidgetInstance = CreateWidget<UPDInventoryWidget>(PC, InventoryWidgetClass);
	}
	if (InventoryWidgetInstance)
	{
		InventoryWidgetInstance->SetActiveStashComponent(StashSource);
		AddWidgetToViewportIfNeeded(InventoryWidgetInstance, GameplayOverlayZOrder);
	}

	if (!StashWidgetInstance)
	{
		StashWidgetInstance = CreateWidget<UPDStashWidget>(PC, StashWidgetClass);
	}
	if (StashWidgetInstance)
	{
		StashWidgetInstance->InitializeStash(StashSource);
		AddWidgetToViewportIfNeeded(StashWidgetInstance, GameplayOverlayZOrder);
	}

	SetGameplayInputBlockedByModalUI(true, StashWidgetInstance);
}

void UPDPlayerUIManagerComponent::CloseStash()
{
	UPDStashComponent* ClosedStashComponent = ActiveStashComponent.Get();

	if (StashWidgetInstance)
	{
		StashWidgetInstance->RemoveFromParent();
		StashWidgetInstance = nullptr;
	}

	if (InventoryWidgetInstance && !IsMarketOpen() && !IsEquipmentModificationOpen())
	{
		InventoryWidgetInstance->RemoveFromParent();
		InventoryWidgetInstance = nullptr;
	}

	ActiveStashComponent.Reset();

	if (!IsMarketOpen() && !IsQuestOpen() && !IsEquipmentModificationOpen())
	{
		SetGameplayInputBlockedByModalUI(false);
	}

	if (ClosedStashComponent)
	{
		if (APDPlayerController* PC = GetPDController())
		{
			PC->OnStashInterfaceClosed.Broadcast(ClosedStashComponent);
		}
	}
}

bool UPDPlayerUIManagerComponent::IsStashOpen() const
{
	return StashWidgetInstance && StashWidgetInstance->IsInViewport();
}

void UPDPlayerUIManagerComponent::OpenInventoryForLoot()
{
	APDPlayerController* PC = GetPDController();
	if (!PC || !PC->IsLocalController()) return;
	if (!InventoryWidgetClass)
	{
		UE_LOG(LogPDCharacter, Warning, TEXT("OpenInventoryForLoot: InventoryWidgetClass is not set."));
		return;
	}

	if (!InventoryWidgetInstance)
	{
		InventoryWidgetInstance = CreateWidget<UPDInventoryWidget>(PC, InventoryWidgetClass);
	}
	if (InventoryWidgetInstance)
	{
		AddWidgetToViewportIfNeeded(InventoryWidgetInstance, GameplayOverlayZOrder);
		InventoryWidgetInstance->SetLootCompanionMode(true);
	}
}

void UPDPlayerUIManagerComponent::CloseInventoryForLoot()
{
	// Loot ?�행 RenderTransform 먼�? 리셋 ???�른 UI 가 ?�벤?�리 ?��??�면 그�?�?보이므�??�위�??�요.
	if (InventoryWidgetInstance)
	{
		InventoryWidgetInstance->SetLootCompanionMode(false);
	}

	// ?�른 UI(Stash/Market/Equip) 가 ?�벤?�리 ?��? ???�면 ?�거.
	if (InventoryWidgetInstance && !IsStashOpen() && !IsMarketOpen() && !IsEquipmentModificationOpen())
	{
		InventoryWidgetInstance->RemoveFromParent();
		InventoryWidgetInstance = nullptr;
	}
}

void UPDPlayerUIManagerComponent::OpenEquipmentModification()
{
	APDPlayerController* PC = GetPDController();
	if (!PC || !PC->IsLocalController()) return;
	if (IsEquipmentModificationOpen()) return;

	CloseStash();
	CloseMarket();
	CloseQuest();

	if (!EquipmentModificationWidgetClass)
	{
		UE_LOG(LogPDCharacter, Warning, TEXT("Equipment modification UI widget class is not set."));
		return;
	}

	EquipmentModificationWidgetInstance = CreateWidget<UUserWidget>(PC, EquipmentModificationWidgetClass);
	if (!EquipmentModificationWidgetInstance)
	{
		UE_LOG(LogPDCharacter, Warning, TEXT("Failed to create equipment modification widget."));
		return;
	}

	AddWidgetToViewportIfNeeded(EquipmentModificationWidgetInstance, GameplayOverlayZOrder);
	SetGameplayInputBlockedByModalUI(true, EquipmentModificationWidgetInstance);
}

void UPDPlayerUIManagerComponent::CloseEquipmentModification()
{
	const bool bWasOpen = IsEquipmentModificationOpen();

	if (EquipmentModificationWidgetInstance)
	{
		EquipmentModificationWidgetInstance->RemoveFromParent();
		EquipmentModificationWidgetInstance = nullptr;
	}

	if (InventoryWidgetInstance && !IsStashOpen() && !IsMarketOpen())
	{
		InventoryWidgetInstance->RemoveFromParent();
		InventoryWidgetInstance = nullptr;
	}

	if (!IsStashOpen() && !IsMarketOpen() && !IsQuestOpen())
	{
		SetGameplayInputBlockedByModalUI(false);
	}

	if (bWasOpen)
	{
		if (APDPlayerController* PC = GetPDController())
		{
			PC->OnEquipmentModificationInterfaceClosed.Broadcast();
		}
	}
}

bool UPDPlayerUIManagerComponent::IsEquipmentModificationOpen() const
{
	return EquipmentModificationWidgetInstance && EquipmentModificationWidgetInstance->IsInViewport();
}

void UPDPlayerUIManagerComponent::OpenQuest()
{
	APDPlayerController* PC = GetPDController();
	if (!PC || !PC->IsLocalController()) return;
	if (IsQuestOpen()) return;

	CloseStash();
	CloseMarket();
	CloseEquipmentModification();

	if (InventoryWidgetInstance)
	{
		InventoryWidgetInstance->RemoveFromParent();
		InventoryWidgetInstance = nullptr;
	}

	if (!QuestWindowWidgetClass)
	{
		UE_LOG(LogPDCharacter, Warning, TEXT("QuestWindowWidgetClass is not set."));
		SetGameplayInputBlockedByModalUI(false);
		return;
	}

	QuestWindowWidgetInstance = CreateWidget<UPDQuestWindowWidget>(PC, QuestWindowWidgetClass);
	if (!QuestWindowWidgetInstance)
	{
		UE_LOG(LogPDCharacter, Warning, TEXT("Failed to create quest window widget."));
		SetGameplayInputBlockedByModalUI(false);
		return;
	}

	QuestWindowWidgetInstance->InitializeQuestWindow(PC->GetPlayerQuestComponent(), PC->GetPlayerInventoryComponent());
	AddWidgetToViewportIfNeeded(QuestWindowWidgetInstance, GameplayOverlayZOrder);
	SetGameplayInputBlockedByModalUI(true, QuestWindowWidgetInstance);
}

void UPDPlayerUIManagerComponent::CloseQuest()
{
	if (QuestWindowWidgetInstance)
	{
		QuestWindowWidgetInstance->RemoveFromParent();
		QuestWindowWidgetInstance = nullptr;
	}

	if (!IsStashOpen() && !IsMarketOpen() && !IsEquipmentModificationOpen())
	{
		SetGameplayInputBlockedByModalUI(false);
	}
}

bool UPDPlayerUIManagerComponent::IsQuestOpen() const
{
	return QuestWindowWidgetInstance && QuestWindowWidgetInstance->IsInViewport();
}

bool UPDPlayerUIManagerComponent::IsHubOpen() const
{
	APDPlayerController* PC = GetPDController();
	if (!PC || !HubScreenClass)
	{
		return false;
	}

	UPDFrontendUISubsystem* Subsystem = UPDFrontendUISubsystem::Get(PC);
	if (!Subsystem)
	{
		return false;
	}

	UPDActivatableBase* Top = Subsystem->GetTopOfLayer(EUILayer::GameMenu);
	return Top && Top->IsA(HubScreenClass);
}

void UPDPlayerUIManagerComponent::ToggleHub()
{
	APDPlayerController* PC = GetPDController();

	if (!PC || !PC->IsLocalController())
	{
		return;
	}

	// PS가 ?�제 �??�착?�을 ???�으므�?�??��?마다 멱등 바인??보강.
	EnsureRaidParticipationBinding();

	// ?�어�?컨텍?�트(Stash/Market/Equip)가 ?�려?�으�?그것부???�는??2?�계 close UX).
	if (IsStashOpen()) { CloseStash(); return; }
	if (IsMarketOpen()) { CloseMarket(); return; }
	if (IsEquipmentModificationOpen()) { CloseEquipmentModification(); return; }

	if (!HubScreenClass)
	{
		UE_LOG(LogPDCharacter, Warning, TEXT("HubScreenClass is not set."));
		return;
	}

	UPDFrontendUISubsystem* Subsystem = UPDFrontendUISubsystem::Get(PC);
	if (!Subsystem)
	{
		return;
	}

	if (IsHubOpen())
	{
		if (UPDActivatableBase* Top = Subsystem->GetTopOfLayer(EUILayer::GameMenu))
		{
			if (const UPDTabbedScreenBase* Hub = Cast<UPDTabbedScreenBase>(Top))
			{
				LastHubTabId = Hub->GetActiveTabId();
			}
		}
		Subsystem->PopFromLayer(EUILayer::GameMenu);
		if (HUDInstance)
		{
			HUDInstance->SetVisibility(ESlateVisibility::Visible);
		}
		return;
	}

	// ?�망/Downed/관??중에???�로 ?�기 거�?. ?�기???�에???��? 처리??
	if (!CanOpenHub())
	{
		return;
	}

	// 마�?�???�� ?�으�?�??�그 ?�달 ??base가 DA??DefaultTabId�?fallback.
	UPDTabbedScreenBase::OpenAtTab(PC, HubScreenClass, LastHubTabId, EUILayer::GameMenu);
	if (HUDInstance)
	{
		HUDInstance->SetVisibility(ESlateVisibility::Collapsed);
	}
}

bool UPDPlayerUIManagerComponent::CanOpenHub() const
{
	const APDPlayerController* PC = GetPDController();
	if (!PC)
	{
		return false;
	}

	if (PC->IsSpectating())
	{
		return false;
	}

	if (const APDPlayerState* PS = PC->GetPlayerState<APDPlayerState>())
	{
		if (PS->IsRaidDead() || PS->IsExtracted())
		{
			return false;
		}
	}

	if (const APDCharacterBase* PawnChar = Cast<APDCharacterBase>(PC->GetPawn()))
	{
		if (PawnChar->IsDowned() || PawnChar->IsDead() || PawnChar->IsGettingUp())
		{
			return false;
		}
	}

	return true;
}

void UPDPlayerUIManagerComponent::EnsureRaidParticipationBinding()
{
	const APDPlayerController* PC = GetPDController();
	if (!PC || !PC->IsLocalController())
	{
		return;
	}

	APDPlayerState* PS = PC->GetPlayerState<APDPlayerState>();
	if (!PS)
	{
		return; // PS가 ?�직 ?�착 ???????�음 ToggleHub ???�시??
	}

	if (BoundPlayerState.Get() == PS)
	{
		return; // ?��? 바인?�됨.
	}

	if (APDPlayerState* Prev = BoundPlayerState.Get())
	{
		Prev->OnRaidParticipationChanged.RemoveDynamic(this, &UPDPlayerUIManagerComponent::HandleRaidParticipationChanged);
	}

	PS->OnRaidParticipationChanged.AddDynamic(this, &UPDPlayerUIManagerComponent::HandleRaidParticipationChanged);
	BoundPlayerState = PS;
}

void UPDPlayerUIManagerComponent::UnbindRaidParticipation()
{
	if (APDPlayerState* PS = BoundPlayerState.Get())
	{
		PS->OnRaidParticipationChanged.RemoveDynamic(this, &UPDPlayerUIManagerComponent::HandleRaidParticipationChanged);
	}
	BoundPlayerState.Reset();
}

void UPDPlayerUIManagerComponent::HandleRaidParticipationChanged(bool bIsExtracted, bool bIsRaidDead)
{
	// Reset(????false) ?�점?�는 ?�을 ?�요 ?�음. ?�망/추출 ?�환 ?�에�??�리.
	if (!bIsExtracted && !bIsRaidDead)
	{
		return;
	}

	// ?�어�?컨텍?�트??base ?�정?�라 raid 중엔 ?�려?��? ?�을 가?�성???��?�??�전?�게 모두 ?�리.
	if (IsStashOpen()) { CloseStash(); }
	if (IsMarketOpen()) { CloseMarket(); }
	if (IsEquipmentModificationOpen()) { CloseEquipmentModification(); }

	if (!IsHubOpen())
	{
		return;
	}

	APDPlayerController* PC = GetPDController();
	if (!PC)
	{
		return;
	}

	UPDFrontendUISubsystem* Subsystem = UPDFrontendUISubsystem::Get(PC);
	if (!Subsystem)
	{
		return;
	}

	if (UPDActivatableBase* Top = Subsystem->GetTopOfLayer(EUILayer::GameMenu))
	{
		if (const UPDTabbedScreenBase* Hub = Cast<UPDTabbedScreenBase>(Top))
		{
			LastHubTabId = Hub->GetActiveTabId();
		}
	}
	Subsystem->PopFromLayer(EUILayer::GameMenu);
	if (HUDInstance)
	{
		HUDInstance->SetVisibility(ESlateVisibility::Visible);
	}
}

void UPDPlayerUIManagerComponent::ShowNotification(const FText& Message, float Duration)
{
	APDPlayerController* PC = GetPDController();
	if (!PC || !PC->IsLocalController() || Message.IsEmpty() || !NotificationWidgetClass)
	{
		return;
	}

	if (!NotificationWidgetInstance)
	{
		NotificationWidgetInstance = CreateWidget<UPDNotificationWidget>(PC, NotificationWidgetClass);
	}
	if (!NotificationWidgetInstance)
	{
		return;
	}

	AddWidgetToViewportIfNeeded(NotificationWidgetInstance, NotificationZOrder);
	NotificationWidgetInstance->ShowNotification(Message, Duration > 0.f ? Duration : NotificationDuration);
}

bool UPDPlayerUIManagerComponent::ShouldAllowMovementWhileUIOpen() const
{
	return InventoryWidgetInstance && InventoryWidgetInstance->IsInViewport()
		&& !IsStashOpen()
		&& !IsMarketOpen()
		&& !IsEquipmentModificationOpen()
		&& !IsQuestOpen();
}

void UPDPlayerUIManagerComponent::SetGameplayInputBlockedByModalUI(bool bBlocked, UUserWidget* WidgetToFocus)
{
	APDPlayerController* PC = GetPDController();
	if (!PC)
	{
		return;
	}

	if (bBlocked)
	{
		if (!bIsGameplayInputBlockedByModalUI)
		{
			bMouseCursorVisibleBeforeModalUI = PC->bShowMouseCursor;
			bMouseClickEventsEnabledBeforeModalUI = PC->bEnableClickEvents;
			bMouseOverEventsEnabledBeforeModalUI = PC->bEnableMouseOverEvents;
		}

		bIsGameplayInputBlockedByModalUI = true;
		PC->OnFireReleased();
		const bool bAllowMovement = ShouldAllowMovementWhileUIOpen();
		PC->SetIgnoreMoveInput(!bAllowMovement);
		PC->SetIgnoreLookInput(true);

		if (!bAllowMovement)
		{
			if (APawn* ControlledPawn = PC->GetPawn())
			{
				if (UPawnMovementComponent* MovementComponent = ControlledPawn->GetMovementComponent())
				{
					MovementComponent->StopMovementImmediately();
				}
			}
		}

		FInputModeGameAndUI InputMode;
		if (WidgetToFocus)
		{
			InputMode.SetWidgetToFocus(WidgetToFocus->TakeWidget());
		}
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(InputMode);

		PC->bShowMouseCursor = true;
		PC->bEnableClickEvents = true;
		PC->bEnableMouseOverEvents = true;
		PC->DefaultMouseCursor = EMouseCursor::Default;
		PC->CurrentMouseCursor = EMouseCursor::Default;

		if (HUDInstance)
		{
			HUDInstance->SetCrosshairVisible(false);
		}
		return;
	}

	if (!bIsGameplayInputBlockedByModalUI)
	{
		return;
	}

	bIsGameplayInputBlockedByModalUI = false;
	PC->SetIgnoreMoveInput(false);
	PC->SetIgnoreLookInput(false);

	FInputModeGameOnly InputMode;
	PC->SetInputMode(InputMode);
	PC->bShowMouseCursor = bMouseCursorVisibleBeforeModalUI;
	PC->bEnableClickEvents = bMouseClickEventsEnabledBeforeModalUI;
	PC->bEnableMouseOverEvents = bMouseOverEventsEnabledBeforeModalUI;

	if (HUDInstance)
	{
		HUDInstance->SetCrosshairVisible(ShouldShowCrosshairForController(PC));
	}
}

void UPDPlayerUIManagerComponent::RefreshNewQuickSlots()
{
	APDPlayerController* PC = GetPDController();
	if (!PC || !PC->IsLocalController())
	{
		return;
	}

	if (!HUDInstance)
	{
		CreateAndAddHUDWidget();
	}

	if (HUDInstance)
	{
		HUDInstance->RefreshNewQuickSlots();
	}
}

void UPDPlayerUIManagerComponent::UpdateCrosshair(const FVector2D& MousePosition, float Spread)
{
	if (HUDInstance)
	{
		HUDInstance->UpdateCrosshair(MousePosition, Spread);
	}
}

void UPDPlayerUIManagerComponent::SetCrosshairType(APDWeaponBase* NewWeapon)
{
	if (!HUDInstance)
	{
		return;
	}

	if (!NewWeapon)
	{
		HUDInstance->SetCrosshairType(EWeaponType::None);
		if (!bIsGameplayInputBlockedByModalUI)
		{
			HUDInstance->SetCrosshairVisible(true);
		}
		return;
	}

	HUDInstance->SetCrosshairType(NewWeapon->GetWeaponType());
	if (!bIsGameplayInputBlockedByModalUI)
	{
		HUDInstance->SetCrosshairVisible(true);
	}
}

void UPDPlayerUIManagerComponent::ToggleWorldMap()
{
	APDPlayerController* PC = GetPDController();
	if (!PC || !PC->IsLocalController()) return;

	if (WorldMapInstance)
	{
		WorldMapInstance->RemoveFromParent();
		WorldMapInstance = nullptr;
		SetGameplayInputBlockedByModalUI(false);
		return;
	}

	if (!WorldMapClass) return;

	WorldMapInstance = CreateWidget<UPDWorldMapWidget>(PC, WorldMapClass);
	if (WorldMapInstance)
	{
		AddWidgetToViewportIfNeeded(WorldMapInstance, WorldMapViewportZOrder);
		SetGameplayInputBlockedByModalUI(true, WorldMapInstance);
	}
}
