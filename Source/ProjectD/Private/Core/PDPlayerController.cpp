#include "Core/PDPlayerController.h"
#include "Characters/PDPlayerCharacter.h"

#include "AbilitySystemComponent.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"

#include "GameplayTag/PDGameplayTags.h"
#include "InputActionValue.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Core/PDGameMode.h"
#include "Core/PDPlayerState.h"
#include "Core/PDPlayerUIManagerComponent.h"
#include "Engine/LocalPlayer.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Ability/PDGameplayAbilityBase.h"
#include "Ability/PDSprintAbility.h"
#include "GameFramework/Character.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Input/PDInputComponent.h"

#include "InputCoreTypes.h"
#include "Widgets/Inventory/PDInventoryWidget.h"
#include "Widgets/Inventory/PDStashWidget.h"
#include "Widgets/Inventory/PDLootWidget.h"
#include "Items/PDLootComponent.h"
#include "Widgets/Inventory/PDMarketWidget.h"
#include "Widgets/Quest/PDQuestWindowWidget.h"
#include "Items/PDMarketComponent.h"
#include "Items/PDInventoryComponent.h"
#include "Items/PDEquipmentComponent.h"
#include "Items/PDQuickSlotComponent.h"
#include "Data/PDQuestComponent.h"
#include "Items/PDStashComponent.h"
#include "Widgets/HUD/PDHUDWidget.h"
#include "Widgets/PDActivatableBase.h"
#include "Widgets/PDRootLayout.h"
#include "Widgets/PDNotificationWidget.h"
#include "Subsystems/PDFrontendUISubsystem.h"
#include "Blueprint/UserWidget.h"

DEFINE_LOG_CATEGORY(LogPDCharacter);

#include "Interfaces/PDInteractable.h"
#include "Weapons/Base/PDWeaponBase.h"
#include "Weapons/Base/PDRangedWeaponBase.h"
#include "Weapons/PDRifle.h"

#include "Ping/PDPingSubsystem.h"
#include "Ping/PDPingInputComponent.h"
#include "Widgets/HUD/PDWorldMapWidget.h"

APDPlayerController::APDPlayerController()
{
	PrimaryActorTick.bCanEverTick=true;
	PingInputComp = CreateDefaultSubobject<UPDPingInputComponent>(TEXT("PingInputComp"));
	UIManagerComponent = CreateDefaultSubobject<UPDPlayerUIManagerComponent>(TEXT("UIManagerComponent"));
}

void APDPlayerController::RequestExtraction()
{
	if (!HasAuthority())
	{
		ServerRequestExtraction();
		return;
	}

	if (APDGameMode* GM=GetWorld()->GetAuthGameMode<APDGameMode>())
	{
		GM->RequestExtraction(this);
	}
}

void APDPlayerController::ServerRequestExtraction_Implementation()
{
	RequestExtraction();
}

APDPlayerState* APDPlayerController::GetPDPlayerState() const
{
	return GetPlayerState<APDPlayerState>();
}

UPDInventoryComponent* APDPlayerController::GetPlayerInventoryComponent() const
{
	if (APDPlayerState* PDPlayerState = GetPDPlayerState())
	{
		return PDPlayerState->GetInventoryComponent();
	}
	return GetPawn() ? GetPawn()->FindComponentByClass<UPDInventoryComponent>() : nullptr;
}

UPDEquipmentComponent* APDPlayerController::GetPlayerEquipmentComponent() const
{
	if (APDPlayerState* PDPlayerState = GetPDPlayerState())
	{
		return PDPlayerState->GetEquipmentComponent();
	}
	return GetPawn() ? GetPawn()->FindComponentByClass<UPDEquipmentComponent>() : nullptr;
}

UPDQuickSlotComponent* APDPlayerController::GetPlayerQuickSlotComponent() const
{
	if (APDPlayerState* PDPlayerState = GetPDPlayerState())
	{
		return PDPlayerState->GetQuickSlotComponent();
	}
	return GetPawn() ? GetPawn()->FindComponentByClass<UPDQuickSlotComponent>() : nullptr;
}

UPDQuestComponent* APDPlayerController::GetPlayerQuestComponent() const
{
	if (APDPlayerState* PDPlayerState = GetPDPlayerState())
	{
		return PDPlayerState->GetQuestComponent();
	}
	return GetPawn() ? GetPawn()->FindComponentByClass<UPDQuestComponent>() : nullptr;
}
void APDPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	TickRecoilRecovery(DeltaTime);
	UpdateAimRotation();
	UpdateCrosshair();
}

void APDPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (DefaultMappingContext)
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	UPDInputComponent* PDIC = CastChecked<UPDInputComponent>(InputComponent);

	if (!InputConfig)
	{
		InputComponent->BindKey(EKeys::Tab, IE_Pressed, this, &APDPlayerController::ToggleInventory);
		InputComponent->BindKey(EKeys::Q, IE_Pressed, this, &APDPlayerController::ToggleQuest);
		InputComponent->BindKey(EKeys::E, IE_Pressed, this, &APDPlayerController::TryInteract);
		InputComponent->BindKey(EKeys::One, IE_Pressed, this, &APDPlayerController::OnQuickslot1);
		InputComponent->BindKey(EKeys::Two, IE_Pressed, this, &APDPlayerController::OnQuickslot2);
		InputComponent->BindKey(EKeys::Three, IE_Pressed, this, &APDPlayerController::OnQuickslot3);
		InputComponent->BindKey(EKeys::Four, IE_Pressed, this, &APDPlayerController::OnQuickslot4);
		InputComponent->BindKey(EKeys::Five, IE_Pressed, this, &APDPlayerController::OnQuickslot5);
		InputComponent->BindKey(EKeys::Six, IE_Pressed, this, &APDPlayerController::OnQuickslot6);
		return;
	}

	PDIC->BindNativeAction(InputConfig, PDGameplayTags::Input_Move,
		ETriggerEvent::Triggered, this, &APDPlayerController::OnMove);
	PDIC->BindNativeAction(InputConfig, PDGameplayTags::Input_Jump,
		ETriggerEvent::Started, this, &APDPlayerController::OnJump);

	if (InputConfig->FindNativeInputActionForTag(PDGameplayTags::Input_Inventory))
	{
		PDIC->BindNativeAction(InputConfig, PDGameplayTags::Input_Inventory,
			ETriggerEvent::Started, this, &APDPlayerController::ToggleInventory);
	}
	else
	{
		InputComponent->BindKey(EKeys::Tab, IE_Pressed, this, &APDPlayerController::ToggleInventory);
	}

	if (InputConfig->FindNativeInputActionForTag(PDGameplayTags::Input_Quest))
	{
		PDIC->BindNativeAction(InputConfig, PDGameplayTags::Input_Quest,
			ETriggerEvent::Started, this, &APDPlayerController::ToggleQuest);
	}
	else
	{
		InputComponent->BindKey(EKeys::Q, IE_Pressed, this, &APDPlayerController::ToggleQuest);
	}

	if (InputConfig->FindNativeInputActionForTag(PDGameplayTags::Input_Interact))
	{
		PDIC->BindNativeAction(InputConfig, PDGameplayTags::Input_Interact,
			ETriggerEvent::Started, this, &APDPlayerController::TryInteract);
		PDIC->BindNativeAction(InputConfig, PDGameplayTags::Input_Interact,
			ETriggerEvent::Completed, this, &APDPlayerController::StopInteract);
		PDIC->BindNativeAction(InputConfig, PDGameplayTags::Input_Interact,
			ETriggerEvent::Canceled, this, &APDPlayerController::StopInteract);
	}
	else
	{
		InputComponent->BindKey(EKeys::E, IE_Pressed, this, &APDPlayerController::TryInteract);
		InputComponent->BindKey(EKeys::E, IE_Released, this, &APDPlayerController::StopInteract);
	}

	PDIC->BindAbilityActions(InputConfig, this, &APDPlayerController::OnAbilityInputPressed,
		&APDPlayerController::OnAbilityInputReleased);

	PDIC->BindNativeAction(InputConfig, PDGameplayTags::Input_Fire,
		ETriggerEvent::Started, this, &APDPlayerController::OnFirePressed);
	PDIC->BindNativeAction(InputConfig, PDGameplayTags::Input_Fire,
		ETriggerEvent::Completed, this, &APDPlayerController::OnFireReleased);
	PDIC->BindNativeAction(InputConfig, PDGameplayTags::Input_Reload,
		ETriggerEvent::Started, this, &APDPlayerController::OnReload);
	PDIC->BindNativeAction(InputConfig, PDGameplayTags::Input_SwitchSlot1,
		ETriggerEvent::Started, this, &APDPlayerController::OnSwitchSlot1);
	PDIC->BindNativeAction(InputConfig, PDGameplayTags::Input_SwitchSlot2,
		ETriggerEvent::Started, this, &APDPlayerController::OnSwitchSlot2);
	PDIC->BindNativeAction(InputConfig, PDGameplayTags::Input_SwitchSlot3,
		ETriggerEvent::Started, this, &APDPlayerController::OnSwitchSlot3);

	InputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed,
		this, &APDPlayerController::OnAimPressed);
	PDIC->BindNativeAction(InputConfig, PDGameplayTags::Input_ToggleFireMode,
		ETriggerEvent::Started, this, &APDPlayerController::OnToggleFireMode);
	PDIC->BindNativeAction(InputConfig, PDGameplayTags::Input_DropWeapon,
		ETriggerEvent::Started, this, &APDPlayerController::OnDropWeapon);








	PDIC->BindNativeAction(InputConfig, PDGameplayTags::Input_Quickslot1,
		ETriggerEvent::Started, this, &APDPlayerController::OnQuickslot1);
	PDIC->BindNativeAction(InputConfig, PDGameplayTags::Input_Quickslot2,
		ETriggerEvent::Started, this, &APDPlayerController::OnQuickslot2);
	PDIC->BindNativeAction(InputConfig, PDGameplayTags::Input_Quickslot3,
		ETriggerEvent::Started, this, &APDPlayerController::OnQuickslot3);
	PDIC->BindNativeAction(InputConfig, PDGameplayTags::Input_Quickslot4,
		ETriggerEvent::Started, this, &APDPlayerController::OnQuickslot4);
	PDIC->BindNativeAction(InputConfig, PDGameplayTags::Input_Quickslot5,
		ETriggerEvent::Started, this, &APDPlayerController::OnQuickslot5);
	PDIC->BindNativeAction(InputConfig, PDGameplayTags::Input_Quickslot6,
		ETriggerEvent::Started, this, &APDPlayerController::OnQuickslot6);

	if (InputConfig->FindNativeInputActionForTag(PDGameplayTags::Input_CancelConsumableUse))
	{
		PDIC->BindNativeAction(InputConfig, PDGameplayTags::Input_CancelConsumableUse,
			ETriggerEvent::Started, this, &APDPlayerController::OnCancelConsumableUse);
	}

	if (PingInputComp && InputConfig)
	{
		PingInputComp->BindInputs(PDIC, InputConfig);
	}

	PDIC->BindNativeAction(InputConfig, PDGameplayTags::Input_Map,
		ETriggerEvent::Started, this, &APDPlayerController::OnToggleWorldMap);
}

void APDPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);

	bShowMouseCursor = false;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
	DefaultMouseCursor = EMouseCursor::Default;
	CurrentMouseCursor = EMouseCursor::Default;

	if (APDPlayerCharacter* Ch = Cast<APDPlayerCharacter>(GetPawn()))
	{
		Ch->OnWeaponSwapped.AddDynamic(this, &APDPlayerController::OnWeaponChanged);
	}

	if (UIManagerComponent)
	{
		UIManagerComponent->ConfigureLegacyWidgetClasses(
			HUDClass,
			RootLayoutClass,
			InventoryWidgetClass,
			StashWidgetClass,
			MarketWidgetClass,
			EquipmentModificationWidgetClass,
			NotificationWidgetClass,
			NotificationDuration,
			NotificationZOrder,
			QuestWindowWidgetClass,
			WorldMapClass);
		UIManagerComponent->InitializeUI(this);
	}

	BindInventoryNotifications();
}

void APDPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindInventoryNotifications();
	if (UIManagerComponent)
	{
		UIManagerComponent->ShutdownUI(EndPlayReason);
	}

	Super::EndPlay(EndPlayReason);
}

void APDPlayerController::OnPossess(APawn* InPawn)
{
	UnbindInventoryNotifications();
	Super::OnPossess(InPawn);

	if (UIManagerComponent)
	{
		UAbilitySystemComponent* ASC = InPawn
			? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(InPawn)
			: nullptr;
		UIManagerComponent->RebindHUDToASC(ASC);
	}

	BindInventoryNotifications();
}

void APDPlayerController::OnUnPossess()
{
	UnbindInventoryNotifications();
	if (UIManagerComponent)
	{
		UIManagerComponent->RebindHUDToASC(nullptr);
	}
	Super::OnUnPossess();
}

void APDPlayerController::CreateAndAddHUDWidget()
{
	if (UIManagerComponent)
	{
		UIManagerComponent->CreateAndAddHUDWidget();
	}
}

void APDPlayerController::ApplyEffectiveUIState(EWidgetInputMode Mode)
{
	if (UIManagerComponent)
	{
		UIManagerComponent->ApplyEffectiveUIState(Mode);
	}
}

void APDPlayerController::OnMove(const struct FInputActionValue& Value)
{
	if (IsGameplayInputBlockedByModalUI() && !ShouldAllowMovementWhileUIOpen()) return;

	if (const APDPlayerCharacter* PlayerCharacter = Cast<APDPlayerCharacter>(GetPawn()))
	{
		if (PlayerCharacter->IsDowned() || PlayerCharacter->IsGettingUp() || PlayerCharacter->IsDead()) return;
	}

	UAbilitySystemComponent* ASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn());

	if (ASC && ASC->HasMatchingGameplayTag(PDGameplayTags::State_MeleeAttacking))
		return;

	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn) return;
	const FVector2D MoveInput = Value.Get<FVector2D>();
	ControlledPawn->AddMovementInput(FVector::ForwardVector, MoveInput.Y);
	ControlledPawn->AddMovementInput(FVector::RightVector, MoveInput.X);
}

void APDPlayerController::OnJump()
{
	if (IsGameplayInputBlockedByModalUI()) return;

	if (const APDPlayerCharacter* PlayerCharacter = Cast<APDPlayerCharacter>(GetPawn()))
	{
		if (PlayerCharacter->IsDowned() || PlayerCharacter->IsGettingUp() || PlayerCharacter->IsDead()) return;
	}

	if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetPawn()))
	{
		OwnerCharacter->Jump();
	}
}

void APDPlayerController::OnAbilityInputPressed(FGameplayTag InputTag)
{
	if (const APDPlayerCharacter* PlayerCharacter = Cast<APDPlayerCharacter>(GetPawn()))
	{
		if (PlayerCharacter->IsDowned() || PlayerCharacter->IsGettingUp() || PlayerCharacter->IsDead()) return;
	}

	if (UAbilitySystemComponent* ASC=UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn()))
	{
		if (InputTag == PDGameplayTags::Input_Roll || InputTag == PDGameplayTags::Input_Sprint)
		{
			const bool bActivated = ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(InputTag));
			if (!HasAuthority() && (InputTag == PDGameplayTags::Input_Sprint || !bActivated))
			{
				ServerTryActivateAbilityByTag(InputTag);
			}
			return;
		}
	}

	if (!HasAuthority())
	{
		ServerTryActivateAbilityByTag(InputTag);
		return;
	}

	if (UAbilitySystemComponent* ASC=UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn()))
	{
		ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(InputTag));
	}
}


void APDPlayerController::OnAbilityInputReleased(FGameplayTag InputTag)
{
	if (InputTag == PDGameplayTags::Input_Sprint)
	{
		CancelAbilityByInputTag(InputTag);

		if (!HasAuthority())
		{
			ServerCancelAbilityByTag(InputTag);
		}
	}
}

void APDPlayerController::CancelAbilityByInputTag(FGameplayTag InputTag)
{
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn());
	if (!ASC || !InputTag.IsValid())
	{
		return;
	}

	TArray<FGameplayAbilitySpecHandle> HandlesToCancel;
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (!Spec.IsActive() || !Spec.Ability)
		{
			continue;
		}

		const bool bMatchesInputTag =
			Spec.Ability->GetAssetTags().HasTagExact(InputTag) ||
			Spec.GetDynamicSpecSourceTags().HasTagExact(InputTag);
		const bool bIsSprintAbility =
			InputTag == PDGameplayTags::Input_Sprint &&
			Spec.Ability->IsA<UPDSprintAbility>();

		if (bMatchesInputTag || bIsSprintAbility)
		{
			HandlesToCancel.Add(Spec.Handle);
		}
	}

	for (const FGameplayAbilitySpecHandle& AbilityHandle : HandlesToCancel)
	{
		ASC->CancelAbilityHandle(AbilityHandle);
	}
}

void APDPlayerController::ServerTryActivateAbilityByTag_Implementation(FGameplayTag InputTag)
{
	if (const APDPlayerCharacter* PlayerCharacter = Cast<APDPlayerCharacter>(GetPawn()))
	{
		if (PlayerCharacter->IsDowned() || PlayerCharacter->IsGettingUp() || PlayerCharacter->IsDead()) return;
	}

	if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn()))
	{
		ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(InputTag));
	}
}

void APDPlayerController::ServerCancelAbilityByTag_Implementation(FGameplayTag InputTag)
{
	CancelAbilityByInputTag(InputTag);
}

void APDPlayerController::ServerHandleGameplayEvent_Implementation(FGameplayTag EventTag)
{
	if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn()))
	{
		FGameplayEventData EventData;
		ASC->HandleGameplayEvent(EventTag, &EventData);
	}
}


void APDPlayerController::OpenMarketInterface(UPDMarketComponent* MarketComponent)
{
	if (UIManagerComponent)
	{
		UIManagerComponent->OpenMarket(MarketComponent);
	}
}

void APDPlayerController::ClientOpenMarketInterface_Implementation(UPDMarketComponent* MarketComponent)
{
	if (!UIManagerComponent) return;
	if (UIManagerComponent->IsMarketOpen() && UIManagerComponent->GetActiveMarketComponent() == MarketComponent)
	{
		UIManagerComponent->CloseMarket();
		return;
	}
	UIManagerComponent->OpenMarket(MarketComponent);
}

void APDPlayerController::CloseMarketInterface()
{
	if (UIManagerComponent)
	{
		UIManagerComponent->CloseMarket();
	}
}

bool APDPlayerController::IsMarketInterfaceOpen() const
{
	return UIManagerComponent && UIManagerComponent->IsMarketOpen();
}

void APDPlayerController::OpenStashInterface(UPDStashComponent* StashSource)
{
	if (HasAuthority() && StashSource)
	{
		StashSource->LoadFromPlayerState(GetPlayerState<APDPlayerState>());
	}

	if (UIManagerComponent)
	{
		UIManagerComponent->OpenStash(StashSource);
	}
}

void APDPlayerController::ClientOpenStashInterface_Implementation(UPDStashComponent* StashSource)
{
	if (!UIManagerComponent) return;
	if (UIManagerComponent->IsStashOpen() && UIManagerComponent->GetActiveStashComponent() == StashSource)
	{
		UIManagerComponent->CloseStash();
		return;
	}
	UIManagerComponent->OpenStash(StashSource);
}

void APDPlayerController::CloseStashInterface()
{
	if (UIManagerComponent)
	{
		UIManagerComponent->CloseStash();
	}
}

bool APDPlayerController::IsStashInterfaceOpen() const
{
	return UIManagerComponent && UIManagerComponent->IsStashOpen();
}


UPDStashComponent* APDPlayerController::GetActiveStashComponent() const
{
	return UIManagerComponent ? UIManagerComponent->GetActiveStashComponent() : nullptr;
}

// ─── LootBox 인터페이스 ──────────────────────────────────────────────
// codex 브랜치는 InventoryWidget 라이프사이클을 UIManager 로 위임하므로 develop 구현을 단순화 —
// LootWidget 만 PlayerController 가 직접 다루고, 입력 차단은 기존 헬퍼 재사용.
void APDPlayerController::OpenLootInterface(UPDLootComponent* LootSource)
{
	if (!LootWidgetClass || !LootSource)
	{
		UE_LOG(LogPDCharacter, Warning, TEXT("OpenLootInterface: missing widget class or source"));
		return;
	}

	// 다른 모달 UI 가 같은 viewport 영역을 점유하므로 먼저 닫음.
	if (IsStashInterfaceOpen())  CloseStashInterface();
	if (IsMarketInterfaceOpen()) CloseMarketInterface();

	ActiveLootComponent = LootSource;

	if (!LootWidgetInstance || !LootWidgetInstance->IsInViewport())
	{
		LootWidgetInstance = CreateWidget<UPDLootWidget>(this, LootWidgetClass);
		if (LootWidgetInstance)
		{
			LootWidgetInstance->InitializeLoot(LootSource);
			LootWidgetInstance->AddToViewport();
		}
	}
	else
	{
		LootWidgetInstance->InitializeLoot(LootSource);
	}

	SetGameplayInputBlockedByModalUI(true, LootWidgetInstance);
}

void APDPlayerController::CloseLootInterface()
{
	UPDLootComponent* ClosingLootComponent = ActiveLootComponent.Get();

	if (LootWidgetInstance && LootWidgetInstance->IsInViewport())
	{
		LootWidgetInstance->RemoveFromParent();
	}
	LootWidgetInstance = nullptr;
	ActiveLootComponent.Reset();

	if (ClosingLootComponent)
	{
		OnLootInterfaceClosed.Broadcast(ClosingLootComponent);
	}

	SetGameplayInputBlockedByModalUI(false);
}

bool APDPlayerController::IsLootInterfaceOpen() const
{
	return LootWidgetInstance && LootWidgetInstance->IsInViewport();
}

bool APDPlayerController::SellInventorySlotToActiveMarket(int32 SlotIndex, int32 Quantity)
{
	UPDMarketComponent* ActiveMarket = UIManagerComponent ? UIManagerComponent->GetActiveMarketComponent() : nullptr;
	if (!ActiveMarket || !IsMarketInterfaceOpen())
	{
		return false;
	}

	UPDInventoryComponent* InventoryComponent = GetPlayerInventoryComponent();
	if (!InventoryComponent)
	{
		return false;
	}

	if (!HasAuthority())
	{
		ServerSellInventorySlotToMarket(ActiveMarket, SlotIndex, Quantity);
		return true;
	}

	return ActiveMarket->SellInventorySlot(InventoryComponent, SlotIndex, Quantity);
}

void APDPlayerController::ServerBuyMarketEntry_Implementation(UPDMarketComponent* MarketComponent, int32 EntryIndex, int32 Quantity)
{
	if (!MarketComponent)
	{
		return;
	}

	if (UPDInventoryComponent* InventoryComponent = GetPlayerInventoryComponent())
	{
		MarketComponent->BuyEntry(InventoryComponent, EntryIndex, Quantity);
	}
}

void APDPlayerController::ServerSellInventorySlotToMarket_Implementation(UPDMarketComponent* MarketComponent, int32 SlotIndex, int32 Quantity)
{
	if (!MarketComponent)
	{
		return;
	}

	if (UPDInventoryComponent* InventoryComponent = GetPlayerInventoryComponent())
	{
		MarketComponent->SellInventorySlot(InventoryComponent, SlotIndex, Quantity);
	}
}

void APDPlayerController::ServerUpgradeStash_Implementation(UPDStashComponent* StashComponent)
{
	if (!StashComponent)
	{
		return;
	}

	if (UPDInventoryComponent* InventoryComponent = GetPlayerInventoryComponent())
	{
		StashComponent->UpgradeStash(InventoryComponent);
	}
}

void APDPlayerController::ServerStoreInventorySlotQuantityToStash_Implementation(UPDStashComponent* StashComponent, int32 SourceSlotIndex, int32 TargetStashSlotIndex, int32 Quantity)
{
	if (!StashComponent)
	{
		return;
	}

	if (UPDInventoryComponent* InventoryComponent = GetPlayerInventoryComponent())
	{
		if (TargetStashSlotIndex == INDEX_NONE)
		{
			StashComponent->StoreInventorySlotQuantity(InventoryComponent, SourceSlotIndex, Quantity);
		}
		else
		{
			StashComponent->StoreInventorySlotQuantityToSlot(InventoryComponent, SourceSlotIndex, TargetStashSlotIndex, Quantity);
		}
	}
}

void APDPlayerController::ServerTakeStashSlotQuantity_Implementation(UPDStashComponent* StashComponent, int32 StashSlotIndex, int32 Quantity)
{
	if (!StashComponent)
	{
		return;
	}

	if (UPDInventoryComponent* InventoryComponent = GetPlayerInventoryComponent())
	{
		StashComponent->TakeStashSlotQuantity(InventoryComponent, StashSlotIndex, Quantity);
	}
}

void APDPlayerController::ServerTakeStashSlotQuantityToInventorySlot_Implementation(UPDStashComponent* StashComponent, int32 StashSlotIndex, int32 TargetInventorySlotIndex, int32 Quantity)
{
	if (!StashComponent)
	{
		return;
	}

	if (UPDInventoryComponent* InventoryComponent = GetPlayerInventoryComponent())
	{
		StashComponent->TakeStashSlotQuantityToInventorySlot(InventoryComponent, StashSlotIndex, TargetInventorySlotIndex, Quantity);
	}
}

void APDPlayerController::ServerMoveStashSlotQuantity_Implementation(UPDStashComponent* StashComponent, int32 SourceSlotIndex, int32 TargetSlotIndex, int32 Quantity)
{
	if (StashComponent)
	{
		if (StashComponent->MoveSlotQuantityToSlot(SourceSlotIndex, TargetSlotIndex, Quantity))
		{
			StashComponent->SaveToPlayerState(GetPlayerState<APDPlayerState>());
		}
	}
}


void APDPlayerController::OpenEquipmentModificationInterface()
{
	if (UIManagerComponent)
	{
		UIManagerComponent->OpenEquipmentModification();
	}
}

void APDPlayerController::CloseEquipmentModificationInterface()
{
	if (UIManagerComponent)
	{
		UIManagerComponent->CloseEquipmentModification();
	}
}

bool APDPlayerController::IsEquipmentModificationInterfaceOpen() const
{
	return UIManagerComponent && UIManagerComponent->IsEquipmentModificationOpen();
}

void APDPlayerController::BindInventoryNotifications()
{
	UnbindInventoryNotifications();

	UPDInventoryComponent* InventoryComponent = GetPlayerInventoryComponent();
	if (!InventoryComponent)
	{
		return;
	}

	BoundInventoryNotificationComponent = InventoryComponent;
	BoundInventoryNotificationComponent->OnInventoryMessage.AddUniqueDynamic(this, &APDPlayerController::HandleInventoryMessage);
}

void APDPlayerController::UnbindInventoryNotifications()
{
	if (BoundInventoryNotificationComponent)
	{
		BoundInventoryNotificationComponent->OnInventoryMessage.RemoveDynamic(this, &APDPlayerController::HandleInventoryMessage);
		BoundInventoryNotificationComponent = nullptr;
	}
}

void APDPlayerController::HandleInventoryMessage(const FText& Message)
{
	ShowNotification(Message);
}

void APDPlayerController::ShowNotification(const FText& Message, float Duration)
{
	if (UIManagerComponent)
	{
		UIManagerComponent->ShowNotification(Message, Duration);
	}
}

void APDPlayerController::OpenQuestInterface()
{
	if (UIManagerComponent)
	{
		UIManagerComponent->OpenQuest();
	}
}

void APDPlayerController::CloseQuestInterface()
{
	if (UIManagerComponent)
	{
		UIManagerComponent->CloseQuest();
	}
}

bool APDPlayerController::IsQuestInterfaceOpen() const
{
	return UIManagerComponent && UIManagerComponent->IsQuestOpen();
}

void APDPlayerController::ToggleQuest()
{
	if (UIManagerComponent)
	{
		UIManagerComponent->ToggleQuest();
	}
}

void APDPlayerController::ToggleInventory()
{
	if (UIManagerComponent)
	{
		UIManagerComponent->ToggleInventory();
	}
}

void APDPlayerController::TryInteract()
{
	if (APDPlayerCharacter* PlayerCharacter = Cast<APDPlayerCharacter>(GetPawn()))
	{
		PlayerCharacter->TryInteract();
	}
}

void APDPlayerController::StopInteract()
{
	if (APDPlayerCharacter* PlayerCharacter = Cast<APDPlayerCharacter>(GetPawn()))
	{
		PlayerCharacter->StopInteract();
	}
}

bool APDPlayerController::IsGameplayInputBlockedByModalUI() const
{
	return UIManagerComponent && UIManagerComponent->IsGameplayInputBlockedByModalUI();
}

bool APDPlayerController::ShouldAllowMovementWhileUIOpen() const
{
	return UIManagerComponent && UIManagerComponent->ShouldAllowMovementWhileUIOpen();
}

void APDPlayerController::SetGameplayInputBlockedByModalUI(bool bBlocked, UUserWidget* WidgetToFocus)
{
	if (UIManagerComponent)
	{
		UIManagerComponent->SetGameplayInputBlockedByModalUI(bBlocked, WidgetToFocus);
	}
}
void APDPlayerController::UpdateAimRotation()
{
	if (IsGameplayInputBlockedByModalUI()) return;

	if (UAbilitySystemComponent* ASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn()))
	{
		if (ASC->HasMatchingGameplayTag(PDGameplayTags::State_Rolling)) return;
	}

	APawn* ControlledPawn =GetPawn();
	if (!ControlledPawn) return;
	if (const APDPlayerCharacter* PlayerCharacter = Cast<APDPlayerCharacter>(ControlledPawn))
	{
		if (PlayerCharacter->IsDowned() || PlayerCharacter->IsGettingUp() || PlayerCharacter->IsDead()) return;
	}

	FHitResult AimHit;
	if (!GetHitResultUnderCursor(ECC_Visibility, true, AimHit)) return;

	FHitResult FireAimHit;
	if (!GetRecoiledHitResult(ECC_Visibility, true, FireAimHit))
	{
		FireAimHit = AimHit;
	}

	CachedAimWorldLocation = FireAimHit.Location;
	bHasCachedAimWorldLocation = true;
	if (APDPlayerCharacter* PlayerCharacter = Cast<APDPlayerCharacter>(ControlledPawn))
	{
		PlayerCharacter->SetSharedAimWorldLocation(AimHit.Location);
	}
	if (!HasAuthority())
	{
		ServerSetAimWorldLocation(AimHit.Location, FireAimHit.Location);
	}

	FVector AimDirection=AimHit.Location-ControlledPawn->GetActorLocation();
	AimDirection.Z=0.f;

	if (!AimDirection.IsNearlyZero())
	{
		FRotator AimRot = AimDirection.Rotation();
		SetControlRotation(AimRot);
		ControlledPawn->SetActorRotation(AimRot);
	}
}

void APDPlayerController::OnSwitchSlot1()
{
	UseQuickSlot(0);
}

void APDPlayerController::OnSwitchSlot2()
{
	UseQuickSlot(1);
}

void APDPlayerController::OnSwitchSlot3()
{
	UseQuickSlot(2);
}

void APDPlayerController::OnUseQuickSlot4()
{
	UseQuickSlot(3);
}

void APDPlayerController::OnQuickslot1()
{
	UseQuickSlot(0);
}

void APDPlayerController::OnQuickslot2()
{
	UseQuickSlot(1);
}

void APDPlayerController::OnQuickslot3()
{
	UseQuickSlot(2);
}

void APDPlayerController::OnQuickslot4()
{
	UseQuickSlot(3);
}

void APDPlayerController::OnQuickslot5()
{
	UseQuickSlot(4);
}

void APDPlayerController::OnQuickslot6()
{
	UseQuickSlot(5);
}

void APDPlayerController::OnCancelConsumableUse()
{
	if (IsGameplayInputBlockedByModalUI())
	{
		return;
	}

	if (UPDQuickSlotComponent* QuickSlotComponent = GetPlayerQuickSlotComponent())
	{
		QuickSlotComponent->CancelConsumableUse();
	}
}

void APDPlayerController::SelectQuickslot(int32 Index)
{
	if (IsGameplayInputBlockedByModalUI())
	{
		return;
	}

	if (UPDQuickSlotComponent* QuickSlotComponent = GetPlayerQuickSlotComponent())
	{
		QuickSlotComponent->SetSelectedIndex(Index);
	}
}

void APDPlayerController::OnAimPressed()
{
	if (const APDPlayerCharacter* PlayerCharacter = Cast<APDPlayerCharacter>(GetPawn()))
	{
		if (PlayerCharacter->IsDowned() || PlayerCharacter->IsGettingUp() || PlayerCharacter->IsDead()) return;
	}

	if (UAbilitySystemComponent* ASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn()))
	{
		if (HasAuthority())
		{
			ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(PDGameplayTags::Input_Aim));
		}
		else
		{
			ServerTryActivateAbilityByTag(PDGameplayTags::Input_Aim);
		}
	}
}

void APDPlayerController::OnToggleFireMode()
{
	if (const APDPlayerCharacter* PlayerCharacter = Cast<APDPlayerCharacter>(GetPawn()))
	{
		if (PlayerCharacter->IsDowned() || PlayerCharacter->IsGettingUp() || PlayerCharacter->IsDead()) return;
	}

	if (!HasAuthority())
	{
		ServerToggleFireMode();
		return;
	}

	if (APDPlayerCharacter* Ch = Cast<APDPlayerCharacter>(GetPawn()))
		if (APDWeaponBase* Weapon = Ch->GetCurrentWeapon())
			if (APDRifle* Rifle = Cast<APDRifle>(Weapon))
				Rifle->ToggleFireMode();
}

void APDPlayerController::ServerToggleFireMode_Implementation()
{
	OnToggleFireMode();
}

void APDPlayerController::OnDropWeapon()
{
	if (APDPlayerCharacter* Ch = Cast<APDPlayerCharacter>(GetPawn()))
	{
		if (Ch->IsDowned() || Ch->IsGettingUp() || Ch->IsDead()) return;
		Ch->DropCurrentWeapon();
	}
}

void APDPlayerController::UseQuickSlot(int32 SlotIndex)
{
	if (IsGameplayInputBlockedByModalUI())
	{
		return;
	}

	if (const APDPlayerCharacter* PlayerCharacter = Cast<APDPlayerCharacter>(GetPawn()))
	{
		if (PlayerCharacter->IsDowned() || PlayerCharacter->IsGettingUp() || PlayerCharacter->IsDead()) return;
	}

	if (UPDQuickSlotComponent* QuickSlotComponent = GetPlayerQuickSlotComponent())
	{
		if (QuickSlotComponent->UseQuickSlot(SlotIndex) && UIManagerComponent)
		{
			UIManagerComponent->RefreshNewQuickSlots();
		}
	}
}

void APDPlayerController::OnInteract()
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn) return;

	if (UPDQuickSlotComponent* QuickSlotComponent = GetPlayerQuickSlotComponent())
	{
		QuickSlotComponent->CancelConsumableUse();
	}

	TArray<AActor*> OverlappingActors;
	ControlledPawn->GetOverlappingActors(OverlappingActors);

	AActor* ClosestInteractable = nullptr;
	float ClosestDist = FLT_MAX;

	for (AActor* Actor : OverlappingActors)
	{
		if (!Actor->Implements<UPDInteractable>()) continue;
		if (Actor->GetAttachParentActor() != nullptr) continue;

		float Dist = FVector::Dist(
			ControlledPawn->GetActorLocation(),
			Actor->GetActorLocation());

		if (Dist < ClosestDist)
		{
			ClosestDist = Dist;
			ClosestInteractable = Actor;
		}
	}

	if (ClosestInteractable)
		IPDInteractable::Execute_Interact(ClosestInteractable, ControlledPawn);
}

void APDPlayerController::OnFirePressed()
{
	if (IsGameplayInputBlockedByModalUI()) { OnFireReleased(); return; }

	if (const APDPlayerCharacter* PlayerCharacter = Cast<APDPlayerCharacter>(GetPawn()))
	{
		if (PlayerCharacter->IsDowned() || PlayerCharacter->IsGettingUp() || PlayerCharacter->IsDead())
		{
			OnFireReleased();
			return;
		}
	}

	if (UPDPingSubsystem* PingSys = GetWorld()->GetSubsystem<UPDPingSubsystem>())
		if (PingSys->IsPingActive()) return;

	if (UPDQuickSlotComponent* QuickSlotComponent = GetPlayerQuickSlotComponent())
	{
		QuickSlotComponent->CancelConsumableUse();
	}

	if (UAbilitySystemComponent* ASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn()))
	{
		if (HasAuthority())
		{
			ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(PDGameplayTags::Input_Fire));
		}
		else
		{
			ServerTryActivateAbilityByTag(PDGameplayTags::Input_Fire);
		}
	}
}

void APDPlayerController::OnFireReleased()
{
	if (!HasAuthority())
	{
		ServerHandleGameplayEvent(PDGameplayTags::Input_FireReleased);
		return;
	}
	if (UAbilitySystemComponent* ASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn()))
	{
		FGameplayEventData EventData;
		ASC->HandleGameplayEvent(PDGameplayTags::Input_FireReleased, &EventData);
	}
}

void APDPlayerController::OnReload()
{
	if (const APDPlayerCharacter* PlayerCharacter = Cast<APDPlayerCharacter>(GetPawn()))
	{
		if (PlayerCharacter->IsDowned() || PlayerCharacter->IsGettingUp() || PlayerCharacter->IsDead()) return;
	}

	if (UAbilitySystemComponent* ASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn()))
	{
		if (HasAuthority())
		{
			ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(PDGameplayTags::Input_Reload));
		}
		else
		{
			ServerTryActivateAbilityByTag(PDGameplayTags::Input_Reload);
		}
	}
}

void APDPlayerController::UpdateCrosshair()
{
	if (!UIManagerComponent) return;

	FVector2D MousePos;
	if (!GetRecoiledMousePosition(MousePos)) return;

	const float Spread = RecoilCursorPixelsPerDegree > KINDA_SMALL_NUMBER
		? RecoilCursorOffset.Size() / RecoilCursorPixelsPerDegree
		: RecoilCursorOffset.Size();
	UIManagerComponent->UpdateCrosshair(MousePos, Spread);
}

void APDPlayerController::OnWeaponChanged(APDWeaponBase* NewWeapon, EWeaponSlot Slot)
{
	if (UIManagerComponent)
	{
		UIManagerComponent->SetCrosshairType(NewWeapon);
	}
}

void APDPlayerController::AddRecoilOffset(float YawDelta)
{


	const float RecoilPixels = FMath::Max(0.f, YawDelta) * RecoilCursorPixelsPerDegree;
	const float HorizontalJitter = RecoilPixels * RecoilCursorHorizontalRatio;

	RecoilCursorOffset.X += FMath::FRandRange(-HorizontalJitter, HorizontalJitter);
	RecoilCursorOffset.Y -= RecoilPixels;

	if (MaxRecoilCursorOffset > 0.f)
	{
		const float MaxOffsetSquared = FMath::Square(MaxRecoilCursorOffset);
		if (RecoilCursorOffset.SizeSquared() > MaxOffsetSquared)
		{
			RecoilCursorOffset = RecoilCursorOffset.GetSafeNormal() * MaxRecoilCursorOffset;
		}
	}

	RecoilYawOffset = RecoilCursorPixelsPerDegree > KINDA_SMALL_NUMBER
		? RecoilCursorOffset.Size() / RecoilCursorPixelsPerDegree
		: RecoilCursorOffset.Size();
}

void APDPlayerController::TickRecoilRecovery(float DeltaTime)
{
	if (RecoilCursorOffset.IsNearlyZero())
	{
		RecoilCursorOffset = FVector2D::ZeroVector;
		RecoilYawOffset = 0.f;
		return;
	}

	const float RecoveryPixels = RecoilRecoverySpeed * RecoilCursorPixelsPerDegree * DeltaTime;
	const float CurrentSize = RecoilCursorOffset.Size();
	if (CurrentSize <= RecoveryPixels || RecoveryPixels <= 0.f)
	{
		RecoilCursorOffset = FVector2D::ZeroVector;
		RecoilYawOffset = 0.f;
		return;
	}

	RecoilCursorOffset -= RecoilCursorOffset.GetSafeNormal() * RecoveryPixels;
	RecoilYawOffset = RecoilCursorPixelsPerDegree > KINDA_SMALL_NUMBER
		? RecoilCursorOffset.Size() / RecoilCursorPixelsPerDegree
		: RecoilCursorOffset.Size();

}

bool APDPlayerController::GetRecoiledMousePosition(FVector2D& OutMousePosition) const
{
	float MouseX = 0.f;
	float MouseY = 0.f;
	if (!GetMousePosition(MouseX, MouseY))
	{
		return false;
	}

	OutMousePosition = FVector2D(MouseX, MouseY) + RecoilCursorOffset;
	return true;
}

bool APDPlayerController::GetRecoiledHitResult(ECollisionChannel TraceChannel, bool bTraceComplex, FHitResult& OutHit) const
{
	FVector2D MousePos;
	if (!GetRecoiledMousePosition(MousePos))
	{
		return false;
	}

	return GetHitResultAtScreenPosition(MousePos, TraceChannel, bTraceComplex, OutHit);
}

bool APDPlayerController::GetRecoiledHitResultForObjects(const TArray<TEnumAsByte<EObjectTypeQuery>>& ObjectTypes, bool bTraceComplex, FHitResult& OutHit) const
{
	FVector2D MousePos;
	if (!GetRecoiledMousePosition(MousePos))
	{
		return false;
	}

	return GetHitResultAtScreenPosition(MousePos, ObjectTypes, bTraceComplex, OutHit);
}

bool APDPlayerController::GetCachedAimWorldLocation(FVector& OutLocation) const
{
	if (!bHasCachedAimWorldLocation)
	{
		return false;
	}

	OutLocation = CachedAimWorldLocation;
	return true;
}

void APDPlayerController::ServerSetAimWorldLocation_Implementation(FVector AimLocation, FVector FireAimLocation)
{
	CachedAimWorldLocation = FireAimLocation;
	bHasCachedAimWorldLocation = true;
	if (APDPlayerCharacter* PlayerCharacter = Cast<APDPlayerCharacter>(GetPawn()))
	{
		if (PlayerCharacter->IsDead()) return;

		PlayerCharacter->SetSharedAimWorldLocation(AimLocation);
		FVector AimDirection = AimLocation - PlayerCharacter->GetActorLocation();
		AimDirection.Z = 0.f;
		if (!AimDirection.IsNearlyZero())
		{
			const FRotator AimRot = AimDirection.Rotation();
			SetControlRotation(AimRot);
			PlayerCharacter->SetActorRotation(AimRot);
		}
	}
}

void APDPlayerController::OnToggleWorldMap()
{
	if (UIManagerComponent)
	{
		UIManagerComponent->ToggleWorldMap();
	}
}
