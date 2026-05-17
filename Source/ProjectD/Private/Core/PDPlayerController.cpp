#include "Core/PDPlayerController.h"
#include "Characters/PDPlayerCharacter.h"

#include "AbilitySystemComponent.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"

#include "GameplayTag/PDGameplayTags.h"
#include "InputActionValue.h"
#include "EnhancedInputSubsystems.h"
#include "Core/PDGameMode.h"
#include "Engine/LocalPlayer.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Ability/PDGameplayAbilityBase.h"
#include "GameFramework/Character.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Input/PDInputComponent.h"

#include "InputCoreTypes.h"
#include "Widgets/Inventory/PDInventoryWidget.h"
#include "Widgets/Inventory/PDStashWidget.h"
#include "Widgets/Inventory/PDMarketWidget.h"
#include "Widgets/Quest/PDQuestWindowWidget.h"
#include "Items/PDMarketComponent.h"
#include "Items/PDInventoryComponent.h"
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
#include "Weapons/PDRifle.h"                   // OnToggleFireMode - ToggleFireMode 전용

#include "Ping/PDPingSubsystem.h"
#include "Ping/PDPingInputComponent.h"
#include "Widgets/HUD/PDWorldMapWidget.h"
#include "Ability/PDCoverAbility.h"
#include "Ability/PDCoverAimAbility.h"

APDPlayerController::APDPlayerController()
{
	PrimaryActorTick.bCanEverTick=true;
	PingInputComp = CreateDefaultSubobject<UPDPingInputComponent>(TEXT("PingInputComp"));
}

void APDPlayerController::RequestExtraction()
{
	if (APDGameMode* GM=GetWorld()->GetAuthGameMode<APDGameMode>())
	{
		GM->RequestExtraction(this);
	}
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
	}
	else
	{
		InputComponent->BindKey(EKeys::E, IE_Pressed, this, &APDPlayerController::TryInteract);
	}


	UE_LOG(LogPDCharacter, Warning, TEXT("[Input] AbilityInputActions count: %d"), InputConfig->AbilityInputActions.Num());
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
	PDIC->BindNativeAction(InputConfig, PDGameplayTags::Input_Cover,
		ETriggerEvent::Started, this, &APDPlayerController::OnCoverPressed);
	PDIC->BindNativeAction(InputConfig, PDGameplayTags::Input_ToggleFireMode,
		ETriggerEvent::Started, this, &APDPlayerController::OnToggleFireMode);
	PDIC->BindNativeAction(InputConfig, PDGameplayTags::Input_DropWeapon,
		ETriggerEvent::Started, this, &APDPlayerController::OnDropWeapon);

	// IA_Quickslot1~6의 IMC 매핑(One~Six)과 충돌. 신규 BindNativeAction 경로로 일원화 (QS-5a).
	// 안정 검증 후 제거 예정.
	// InputComponent->BindKey(EKeys::One, IE_Pressed, this, &APDPlayerController::OnSwitchSlot1);
	// InputComponent->BindKey(EKeys::Two, IE_Pressed, this, &APDPlayerController::OnSwitchSlot2);
	// InputComponent->BindKey(EKeys::Three, IE_Pressed, this, &APDPlayerController::OnSwitchSlot3);
	// InputComponent->BindKey(EKeys::Four, IE_Pressed, this, &APDPlayerController::OnUseQuickSlot4);

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
	
	//월드맵
	PDIC->BindNativeAction(InputConfig, PDGameplayTags::Input_Map,
	ETriggerEvent::Started, this, &APDPlayerController::OnToggleWorldMap);
}

void APDPlayerController::BeginPlay()
{
	Super::BeginPlay();

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
		Ch->OnWeaponSwapped.AddDynamic(this, &APDPlayerController::OnWeaponChanged);

	CreateAndAddHUDWidget();
	BindInventoryNotifications();
	
	if (RootLayoutClass)
	{
		RootLayoutInstance = CreateWidget<UPDRootLayout>(this, RootLayoutClass);
		if (RootLayoutInstance)
		{
			RootLayoutInstance->AddToViewport(5);
			if (UPDFrontendUISubsystem* UISubsystem = UPDFrontendUISubsystem::Get(this))
			{
				UISubsystem->OnEffectiveUIStateChanged.AddUObject(this, &APDPlayerController::ApplyEffectiveUIState);
				UISubsystem->RegisterRootLayout(RootLayoutInstance);
			}
		}
	}
}

void APDPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindInventoryNotifications();
	if (QuestWindowWidgetInstance)
	{
		QuestWindowWidgetInstance->RemoveFromParent();
		QuestWindowWidgetInstance = nullptr;
	}

	if (NotificationWidgetInstance)
	{
		NotificationWidgetInstance->RemoveFromParent();
		NotificationWidgetInstance = nullptr;
	}

	if (EquipmentModificationWidgetInstance)
	{
		EquipmentModificationWidgetInstance->RemoveFromParent();
		EquipmentModificationWidgetInstance = nullptr;
	}

	if (HUDInstance)
	{
		HUDInstance->Deactivate();
		HUDInstance->RemoveFromParent();
		HUDInstance = nullptr;
	}

	if (UPDFrontendUISubsystem* UISubsystem = UPDFrontendUISubsystem::Get(this))
	{
		UISubsystem->OnEffectiveUIStateChanged.RemoveAll(this);
		UISubsystem->UnregisterRootLayout();
	}

	if (RootLayoutInstance)
	{
		RootLayoutInstance->RemoveFromParent();
		RootLayoutInstance = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void APDPlayerController::OnPossess(APawn* InPawn)
{
	UnbindInventoryNotifications();
	Super::OnPossess(InPawn);

	if (HUDInstance)
	{
		UAbilitySystemComponent* ASC = InPawn
			? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(InPawn)
			: nullptr;
		HUDInstance->RebindToASC(ASC);
	}

	BindInventoryNotifications();
}

void APDPlayerController::OnUnPossess()
{
	UnbindInventoryNotifications();
	if (HUDInstance)
	{
		HUDInstance->RebindToASC(nullptr);
	}
	Super::OnUnPossess();
}

void APDPlayerController::CreateAndAddHUDWidget()
{
	if (!HUDClass) return;
	if (HUDInstance) return;

	HUDInstance = CreateWidget<UPDHUDWidget>(this, HUDClass);
	if (HUDInstance)
	{
		HUDInstance->AddToViewport(0);
		HUDInstance->Activate();
	}
}

void APDPlayerController::ApplyEffectiveUIState(EWidgetInputMode Mode)
{
	switch (Mode)
	{
	case EWidgetInputMode::Game:
		{
			FInputModeGameAndUI InputMode;
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			InputMode.SetHideCursorDuringCapture(false);
			SetInputMode(InputMode);
			bShowMouseCursor = false;
			if (HUDInstance)
			{
				HUDInstance->SetCrosshairVisible(true);
			}
			break;
		}
	case EWidgetInputMode::GameAndMenu:
		{
			FInputModeGameAndUI InputMode;
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			InputMode.SetHideCursorDuringCapture(false);
			SetInputMode(InputMode);
			bShowMouseCursor = true;
			if (HUDInstance)
			{
				HUDInstance->SetCrosshairVisible(false);
			}
			break;
		}
	case EWidgetInputMode::Menu:
		{
			FInputModeUIOnly InputMode;
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			SetInputMode(InputMode);
			bShowMouseCursor = true;
			if (HUDInstance)
			{
				HUDInstance->SetCrosshairVisible(false);
			}
			break;
		}
	case EWidgetInputMode::Passive:
		break;
	}
}

void APDPlayerController::OnMove(const struct FInputActionValue& Value)
{
	if (IsGameplayInputBlockedByModalUI() && !ShouldAllowMovementWhileUIOpen()) return;

	UAbilitySystemComponent* ASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn());

	if (ASC && ASC->HasMatchingGameplayTag(PDGameplayTags::State_MeleeAttacking))
		return;

	if (ASC && (ASC->HasMatchingGameplayTag(PDGameplayTags::Cover_Active) ||
	            ASC->HasMatchingGameplayTag(PDGameplayTags::State_CoverAim)))
	{
		TArray<FGameplayAbilitySpecHandle> ToCancel;
		for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
			if (Spec.IsActive()) ToCancel.Add(Spec.Handle);
		for (const FGameplayAbilitySpecHandle& Handle : ToCancel)
			ASC->CancelAbilityHandle(Handle);
		return;
	}

	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn) return;
	const FVector2D MoveInput = Value.Get<FVector2D>();
	ControlledPawn->AddMovementInput(FVector::ForwardVector, MoveInput.Y);
	ControlledPawn->AddMovementInput(FVector::RightVector, MoveInput.X);
}

void APDPlayerController::OnJump()
{
	if (IsGameplayInputBlockedByModalUI()) return;

	if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetPawn()))
	{
		OwnerCharacter->Jump();
	}
}

void APDPlayerController::OnAbilityInputPressed(FGameplayTag InputTag)
{
	if (UAbilitySystemComponent* ASC=UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn()))
	{
		const bool bActivated = ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(InputTag));
	}
}


void APDPlayerController::OnAbilityInputReleased(FGameplayTag InputTag)
{

}


void APDPlayerController::OpenMarketInterface(UPDMarketComponent* MarketComponent)
{
	if (!MarketComponent)
	{
		UE_LOG(LogPDCharacter, Warning, TEXT("MarketComponent is not valid."));
		return;
	}

	if (!InventoryWidgetClass)
	{
		UE_LOG(LogPDCharacter, Warning, TEXT("InventoryWidgetClass is not set."));
		return;
	}

	if (!MarketWidgetClass)
	{
		UE_LOG(LogPDCharacter, Warning, TEXT("MarketWidgetClass is not set."));
		return;
	}

	ActiveMarketComponent = MarketComponent;

	if (IsStashInterfaceOpen())
	{
		CloseStashInterface();
	}

	if (IsEquipmentModificationInterfaceOpen())
	{
		CloseEquipmentModificationInterface();
	}

	if (!InventoryWidgetInstance || !InventoryWidgetInstance->IsInViewport())
	{
		InventoryWidgetInstance = CreateWidget<UPDInventoryWidget>(this, InventoryWidgetClass);
		if (InventoryWidgetInstance)
		{
			InventoryWidgetInstance->AddToViewport();
		}
	}

	if (!MarketWidgetInstance || !MarketWidgetInstance->IsInViewport())
	{
		MarketWidgetInstance = CreateWidget<UPDMarketWidget>(this, MarketWidgetClass);
		if (MarketWidgetInstance)
		{
			MarketWidgetInstance->InitializeMarket(MarketComponent);
			MarketWidgetInstance->AddToViewport();
		}
	}
	else
	{
		MarketWidgetInstance->InitializeMarket(MarketComponent);
	}

	SetGameplayInputBlockedByModalUI(true, MarketWidgetInstance);
}

void APDPlayerController::CloseMarketInterface()
{
	if (MarketWidgetInstance && MarketWidgetInstance->IsInViewport())
	{
		MarketWidgetInstance->RemoveFromParent();
	}
	MarketWidgetInstance = nullptr;
	ActiveMarketComponent = nullptr;

	if (InventoryWidgetInstance && InventoryWidgetInstance->IsInViewport())
	{
		InventoryWidgetInstance->RemoveFromParent();
	}
	InventoryWidgetInstance = nullptr;

	if (!IsStashInterfaceOpen() && !IsQuestInterfaceOpen() && !IsEquipmentModificationInterfaceOpen())
	{
		SetGameplayInputBlockedByModalUI(false);
	}
}

bool APDPlayerController::IsMarketInterfaceOpen() const
{
	return MarketWidgetInstance && MarketWidgetInstance->IsInViewport();
}

void APDPlayerController::OpenStashInterface(UPDStashComponent* StashSource)
{
	if (!InventoryWidgetClass)
	{
		UE_LOG(LogPDCharacter, Warning, TEXT("InventoryWidgetClass is not set."));
		return;
	}

	if (!StashWidgetClass)
	{
		UE_LOG(LogPDCharacter, Warning, TEXT("StashWidgetClass is not set."));
		return;
	}

	if (!StashSource)
	{
		UE_LOG(LogPDCharacter, Warning, TEXT("OpenStashInterface called with null StashSource."));
		return;
	}

	if (IsMarketInterfaceOpen())
	{
		CloseMarketInterface();
	}

	if (IsEquipmentModificationInterfaceOpen())
	{
		CloseEquipmentModificationInterface();
	}

	ActiveStashComponent = StashSource;

	if (!InventoryWidgetInstance || !InventoryWidgetInstance->IsInViewport())
	{
		InventoryWidgetInstance = CreateWidget<UPDInventoryWidget>(this, InventoryWidgetClass);
		if (InventoryWidgetInstance)
		{
			InventoryWidgetInstance->SetActiveStashComponent(StashSource);
			InventoryWidgetInstance->AddToViewport();
		}
	}
	else
	{
		InventoryWidgetInstance->SetActiveStashComponent(StashSource);
	}

	if (!StashWidgetInstance || !StashWidgetInstance->IsInViewport())
	{
		StashWidgetInstance = CreateWidget<UPDStashWidget>(this, StashWidgetClass);
		if (StashWidgetInstance)
		{
			StashWidgetInstance->InitializeStash(StashSource);
			StashWidgetInstance->AddToViewport();
		}
	}
	else
	{
		// 다른 박스로 열기를 다시 누른 경우: 위젯은 유지하고 데이터만 교체.
		StashWidgetInstance->InitializeStash(StashSource);
	}

	SetGameplayInputBlockedByModalUI(true, StashWidgetInstance);
}

void APDPlayerController::CloseStashInterface()
{
	if (StashWidgetInstance && StashWidgetInstance->IsInViewport())
	{
		StashWidgetInstance->RemoveFromParent();
	}
	StashWidgetInstance = nullptr;

	if (InventoryWidgetInstance && InventoryWidgetInstance->IsInViewport())
	{
		InventoryWidgetInstance->RemoveFromParent();
	}
	InventoryWidgetInstance = nullptr;

	ActiveStashComponent.Reset();

	if (!IsMarketInterfaceOpen() && !IsQuestInterfaceOpen() && !IsEquipmentModificationInterfaceOpen())
	{
		SetGameplayInputBlockedByModalUI(false);
	}
}

bool APDPlayerController::IsStashInterfaceOpen() const
{
	return StashWidgetInstance && StashWidgetInstance->IsInViewport();
}


bool APDPlayerController::SellInventorySlotToActiveMarket(int32 SlotIndex, int32 Quantity)
{
	if (!ActiveMarketComponent || !IsMarketInterfaceOpen())
	{
		return false;
	}

	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return false;
	}

	UPDInventoryComponent* InventoryComponent = ControlledPawn->FindComponentByClass<UPDInventoryComponent>();
	if (!InventoryComponent)
	{
		return false;
	}

	return ActiveMarketComponent->SellInventorySlot(InventoryComponent, SlotIndex, Quantity);
}


void APDPlayerController::OpenEquipmentModificationInterface()
{
	if (IsEquipmentModificationInterfaceOpen())
	{
		return;
	}

	if (IsStashInterfaceOpen())
	{
		CloseStashInterface();
	}

	if (IsMarketInterfaceOpen())
	{
		CloseMarketInterface();
	}

	if (IsQuestInterfaceOpen())
	{
		CloseQuestInterface();
	}

	if (!InventoryWidgetClass)
	{
		UE_LOG(LogPDCharacter, Warning, TEXT("InventoryWidgetClass is not set."));
		return;
	}

	if (!EquipmentModificationWidgetClass)
	{
		UE_LOG(LogPDCharacter, Warning, TEXT("EquipmentModificationWidgetClass is not set."));
		return;
	}

	if (!InventoryWidgetInstance || !InventoryWidgetInstance->IsInViewport())
	{
		InventoryWidgetInstance = CreateWidget<UPDInventoryWidget>(this, InventoryWidgetClass);
		if (InventoryWidgetInstance)
		{
			InventoryWidgetInstance->AddToViewport();
		}
	}

	EquipmentModificationWidgetInstance = CreateWidget<UUserWidget>(this, EquipmentModificationWidgetClass);
	if (!EquipmentModificationWidgetInstance)
	{
		UE_LOG(LogPDCharacter, Warning, TEXT("Failed to create equipment modification widget."));
		return;
	}

	EquipmentModificationWidgetInstance->AddToViewport();
	SetGameplayInputBlockedByModalUI(true, EquipmentModificationWidgetInstance);
}

void APDPlayerController::CloseEquipmentModificationInterface()
{
	if (EquipmentModificationWidgetInstance && EquipmentModificationWidgetInstance->IsInViewport())
	{
		EquipmentModificationWidgetInstance->RemoveFromParent();
	}
	EquipmentModificationWidgetInstance = nullptr;

	if (InventoryWidgetInstance && InventoryWidgetInstance->IsInViewport())
	{
		InventoryWidgetInstance->RemoveFromParent();
	}
	InventoryWidgetInstance = nullptr;

	if (!IsStashInterfaceOpen() && !IsMarketInterfaceOpen() && !IsQuestInterfaceOpen())
	{
		SetGameplayInputBlockedByModalUI(false);
	}
}

bool APDPlayerController::IsEquipmentModificationInterfaceOpen() const
{
	return EquipmentModificationWidgetInstance && EquipmentModificationWidgetInstance->IsInViewport();
}


void APDPlayerController::BindInventoryNotifications()
{
	UnbindInventoryNotifications();

	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return;
	}

	UPDInventoryComponent* InventoryComponent = ControlledPawn->FindComponentByClass<UPDInventoryComponent>();
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
	if (Message.IsEmpty())
	{
		return;
	}

	if (!NotificationWidgetClass)
	{
		return;
	}

	if (!NotificationWidgetInstance)
	{
		NotificationWidgetInstance = CreateWidget<UPDNotificationWidget>(this, NotificationWidgetClass);
	}

	if (!NotificationWidgetInstance)
	{
		return;
	}

	if (!NotificationWidgetInstance->IsInViewport())
	{
		NotificationWidgetInstance->AddToViewport(NotificationZOrder);
	}

	NotificationWidgetInstance->ShowNotification(Message, Duration > 0.f ? Duration : NotificationDuration);
}

void APDPlayerController::OpenQuestInterface()
{
	if (IsQuestInterfaceOpen())
	{
		return;
	}

	if (IsStashInterfaceOpen())
	{
		CloseStashInterface();
	}

	if (IsMarketInterfaceOpen())
	{
		CloseMarketInterface();
	}

	if (IsEquipmentModificationInterfaceOpen())
	{
		CloseEquipmentModificationInterface();
	}

	if (InventoryWidgetInstance && InventoryWidgetInstance->IsInViewport())
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

	QuestWindowWidgetInstance = CreateWidget<UPDQuestWindowWidget>(this, QuestWindowWidgetClass);
	if (!QuestWindowWidgetInstance)
	{
		UE_LOG(LogPDCharacter, Warning, TEXT("Failed to create quest window widget."));
		SetGameplayInputBlockedByModalUI(false);
		return;
	}

	APawn* ControlledPawn = GetPawn();
	UPDQuestComponent* QuestComponent = ControlledPawn ? ControlledPawn->FindComponentByClass<UPDQuestComponent>() : nullptr;
	UPDInventoryComponent* InventoryComponent = ControlledPawn ? ControlledPawn->FindComponentByClass<UPDInventoryComponent>() : nullptr;
	QuestWindowWidgetInstance->InitializeQuestWindow(QuestComponent, InventoryComponent);
	QuestWindowWidgetInstance->AddToViewport();

	SetGameplayInputBlockedByModalUI(true, QuestWindowWidgetInstance);
}

void APDPlayerController::CloseQuestInterface()
{
	if (QuestWindowWidgetInstance && QuestWindowWidgetInstance->IsInViewport())
	{
		QuestWindowWidgetInstance->RemoveFromParent();
	}
	QuestWindowWidgetInstance = nullptr;

	if (!IsStashInterfaceOpen() && !IsMarketInterfaceOpen() && !IsEquipmentModificationInterfaceOpen())
	{
		SetGameplayInputBlockedByModalUI(false);
	}
}

bool APDPlayerController::IsQuestInterfaceOpen() const
{
	return QuestWindowWidgetInstance && QuestWindowWidgetInstance->IsInViewport();
}

void APDPlayerController::ToggleQuest()
{
	if (IsQuestInterfaceOpen())
	{
		CloseQuestInterface();
		return;
	}

	OpenQuestInterface();
}

void APDPlayerController::ToggleInventory()
{
	if (IsQuestInterfaceOpen())
	{
		CloseQuestInterface();
		return;
	}

	if (IsStashInterfaceOpen())
	{
		CloseStashInterface();
		return;
	}

	if (IsMarketInterfaceOpen())
	{
		CloseMarketInterface();
		return;
	}

	if (IsEquipmentModificationInterfaceOpen())
	{
		CloseEquipmentModificationInterface();
		return;
	}

	if (InventoryWidgetInstance && InventoryWidgetInstance->IsInViewport())
	{
		InventoryWidgetInstance->RemoveFromParent();
		InventoryWidgetInstance = nullptr;

		SetGameplayInputBlockedByModalUI(false);
		return;
	}

	if (!InventoryWidgetClass)
	{
		UE_LOG(LogPDCharacter, Warning, TEXT("InventoryWidgetClass is not set."));
		return;
	}

	InventoryWidgetInstance = CreateWidget<UPDInventoryWidget>(this, InventoryWidgetClass);
	if (!InventoryWidgetInstance)
	{
		UE_LOG(LogPDCharacter, Warning, TEXT("Failed to create inventory widget."));
		return;
	}

	InventoryWidgetInstance->AddToViewport();

	SetGameplayInputBlockedByModalUI(true, InventoryWidgetInstance);
	SetIgnoreMoveInput(false);
}

void APDPlayerController::TryInteract()
{
	if (APDPlayerCharacter* PlayerCharacter = Cast<APDPlayerCharacter>(GetPawn()))
	{
		PlayerCharacter->TryInteract();
	}
}

bool APDPlayerController::IsGameplayInputBlockedByModalUI() const
{
	return bIsGameplayInputBlockedByModalUI;
}

bool APDPlayerController::ShouldAllowMovementWhileUIOpen() const
{
	return InventoryWidgetInstance && InventoryWidgetInstance->IsInViewport()
		&& !IsStashInterfaceOpen()
		&& !IsMarketInterfaceOpen()
		&& !IsEquipmentModificationInterfaceOpen()
		&& !IsQuestInterfaceOpen();
}

void APDPlayerController::SetGameplayInputBlockedByModalUI(bool bBlocked, UUserWidget* WidgetToFocus)
{
	if (bBlocked)
	{
		if (!bIsGameplayInputBlockedByModalUI)
		{
			bMouseCursorVisibleBeforeModalUI = bShowMouseCursor;
			bMouseClickEventsEnabledBeforeModalUI = bEnableClickEvents;
			bMouseOverEventsEnabledBeforeModalUI = bEnableMouseOverEvents;
		}

		bIsGameplayInputBlockedByModalUI = true;
		OnFireReleased();
		const bool bAllowMovement = ShouldAllowMovementWhileUIOpen();
		SetIgnoreMoveInput(!bAllowMovement);
		SetIgnoreLookInput(true);

		if (!bAllowMovement)
		{
			if (APawn* ControlledPawn = GetPawn())
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
		SetInputMode(InputMode);

		bShowMouseCursor = true;
		bEnableClickEvents = true;
		bEnableMouseOverEvents = true;
		DefaultMouseCursor = EMouseCursor::Default;
		CurrentMouseCursor = EMouseCursor::Default;

		// 인벤토리/창고/마켓 같은 모달 UI가 열려 있는 동안에는
		// 게임 조준 크로스헤어를 숨기고, 시스템 마우스 커서만 보이게 한다.
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
	SetIgnoreMoveInput(false);
	SetIgnoreLookInput(false);

	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
	bShowMouseCursor = bMouseCursorVisibleBeforeModalUI;
	bEnableClickEvents = bMouseClickEventsEnabledBeforeModalUI;
	bEnableMouseOverEvents = bMouseOverEventsEnabledBeforeModalUI;

	// 모달 UI가 닫히면 게임 조준 크로스헤어를 다시 표시한다.
	if (HUDInstance)
	{
		HUDInstance->SetCrosshairVisible(true);
	}
}

void APDPlayerController::UpdateAimRotation()
{
	if (IsGameplayInputBlockedByModalUI()) return;

	if (UAbilitySystemComponent* ASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn()))
	{
		if (ASC->HasMatchingGameplayTag(PDGameplayTags::State_Rolling)) return;
		if (ASC->HasMatchingGameplayTag(PDGameplayTags::Cover_Active)) return;
	}

	APawn* ControlledPawn =GetPawn();
	if (!ControlledPawn) return;

	FHitResult Hit;
	if (!GetHitResultUnderCursor(ECC_Visibility, true, Hit)) return;

	FVector AimDirection=Hit.Location-ControlledPawn->GetActorLocation();
	AimDirection.Z=0.f;

	if (!AimDirection.IsNearlyZero())
	{
		FRotator AimRot = AimDirection.Rotation();
		AimRot.Yaw += RecoilYawOffset;
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

	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return;
	}

	if (UPDQuickSlotComponent* QuickSlotComponent = ControlledPawn->FindComponentByClass<UPDQuickSlotComponent>())
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

	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return;
	}

	if (UPDQuickSlotComponent* QuickSlotComponent = ControlledPawn->FindComponentByClass<UPDQuickSlotComponent>())
	{
		QuickSlotComponent->SetSelectedIndex(Index);
	}
}

void APDPlayerController::OnCoverPressed()
{
	UAbilitySystemComponent* ASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn());
	if (!ASC) return;

	// 조준 중이면 → 조준만 해제 (커버 유지)
	if (ASC->HasMatchingGameplayTag(PDGameplayTags::State_CoverAim))
	{
		TArray<FGameplayAbilitySpecHandle> ToCancel;
		for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
			if (Spec.IsActive() && Spec.Ability &&
			    Spec.Ability->IsA<UPDCoverAimAbility>())
				ToCancel.Add(Spec.Handle);
		for (const FGameplayAbilitySpecHandle& Handle : ToCancel)
			ASC->CancelAbilityHandle(Handle);
		return;
	}

	// 그 외: GAS에 위임 (진입 or 해제)
	ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(PDGameplayTags::Input_Cover));
}

void APDPlayerController::OnAimPressed()
{
	if (UAbilitySystemComponent* ASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn()))
	{
		ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(PDGameplayTags::Input_Aim));
	}
}

void APDPlayerController::OnToggleFireMode()
{
	if (APDPlayerCharacter* Ch = Cast<APDPlayerCharacter>(GetPawn()))
		if (APDWeaponBase* Weapon = Ch->GetCurrentWeapon())
			if (APDRifle* Rifle = Cast<APDRifle>(Weapon))
				Rifle->ToggleFireMode();
}

void APDPlayerController::OnDropWeapon()
{
	if (APDPlayerCharacter* Ch = Cast<APDPlayerCharacter>(GetPawn()))
		Ch->DropCurrentWeapon();
}

void APDPlayerController::UseQuickSlot(int32 SlotIndex)
{
	if (IsGameplayInputBlockedByModalUI())
	{
		return;
	}

	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return;
	}

	if (UPDQuickSlotComponent* QuickSlotComponent = ControlledPawn->FindComponentByClass<UPDQuickSlotComponent>())
	{
		if (QuickSlotComponent->UseQuickSlot(SlotIndex) && HUDInstance)
		{
			HUDInstance->RefreshNewQuickSlots();
		}
	}
}

void APDPlayerController::OnInteract()
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn) return;

	if (UPDQuickSlotComponent* QuickSlotComponent = ControlledPawn->FindComponentByClass<UPDQuickSlotComponent>())
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

	if (UPDPingSubsystem* PingSys = GetWorld()->GetSubsystem<UPDPingSubsystem>())
		if (PingSys->IsPingActive()) return;

	if (APawn* ControlledPawn = GetPawn())
	{
		if (UPDQuickSlotComponent* QuickSlotComponent = ControlledPawn->FindComponentByClass<UPDQuickSlotComponent>())
		{
			QuickSlotComponent->CancelConsumableUse();
		}
	}

	if (UAbilitySystemComponent* ASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn()))
		ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(PDGameplayTags::Input_Fire));
}

void APDPlayerController::OnFireReleased()
{
	// GA_FireAbility 가 이 이벤트를 수신해 자동화기 루프를 종료함
	if (UAbilitySystemComponent* ASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn()))
	{
		FGameplayEventData EventData;
		ASC->HandleGameplayEvent(PDGameplayTags::Input_FireReleased, &EventData);
	}
}

void APDPlayerController::OnReload()
{
	if (UAbilitySystemComponent* ASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn()))
		ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(PDGameplayTags::Input_Reload));
}

void APDPlayerController::UpdateCrosshair()
{
	if (!HUDInstance) return;

	float MouseX, MouseY;
	GetMousePosition(MouseX, MouseY);

	// 크로스헤어 확장 = 누적된 Yaw 오프셋 기반
	HUDInstance->UpdateCrosshair(FVector2D(MouseX, MouseY), FMath::Abs(RecoilYawOffset));
}

void APDPlayerController::OnWeaponChanged(APDWeaponBase* NewWeapon, EWeaponSlot Slot)
{
	if (!HUDInstance || !NewWeapon) return;
	HUDInstance->SetCrosshairType(NewWeapon->GetWeaponType());
}

void APDPlayerController::AddRecoilOffset(float YawDelta)
{
	// 무기의 MaxRecoilYaw 클램핑은 무기 쪽에서 이미 처리.
	// 컨트롤러는 단순 누적만.
	RecoilYawOffset += YawDelta;
}

void APDPlayerController::TickRecoilRecovery(float DeltaTime)
{
	if (FMath::IsNearlyZero(RecoilYawOffset)) return;

	const float Sign = RecoilYawOffset > 0.f ? 1.f : -1.f;
	const float Recovery = RecoilRecoverySpeed * DeltaTime;
	RecoilYawOffset -= Sign * Recovery;

	// 0을 지나치면 클램프
	if (Sign > 0.f && RecoilYawOffset < 0.f) RecoilYawOffset = 0.f;
	if (Sign < 0.f && RecoilYawOffset > 0.f) RecoilYawOffset = 0.f;
}

void APDPlayerController::OnToggleWorldMap()
{
	//이미 떠 있으면 닫기
	if (IsValid(WorldMapInstance))
	{
		WorldMapInstance->RemoveFromParent();
		WorldMapInstance = nullptr;
		
		bShowMouseCursor = false;
		return;
	}

	//열기
	if (!WorldMapClass) return;

	WorldMapInstance = CreateWidget<UPDWorldMapWidget>(this, WorldMapClass);
	if (WorldMapInstance)
	{
		WorldMapInstance->AddToViewport(10);
		
		bShowMouseCursor = true;
	}
}
