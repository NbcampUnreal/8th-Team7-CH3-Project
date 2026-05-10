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
#include "GameFramework/Character.h"
#include "Input/PDInputComponent.h"

#include "InputCoreTypes.h"
#include "Widgets/Inventory/PDInventoryWidget.h"
#include "Widgets/Inventory/PDStashWidget.h"
#include "Widgets/Inventory/PDMarketWidget.h"
#include "Items/PDMarketComponent.h"
#include "Items/PDInventoryComponent.h"
#include "Widgets/HUD/PDHUDWidget.h"
#include "Subsystems/PDFrontendUISubsystem.h"

DEFINE_LOG_CATEGORY(LogPDCharacter);

#include "Interfaces/PDInteractable.h"
#include "Weapons/PDWeaponBase.h"
#include "Weapons/PDRifle.h"
#include "Weapons/PDSniper.h"


APDPlayerController::APDPlayerController()
{
	PrimaryActorTick.bCanEverTick=true;
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
	static float LogTimer = 0.f;
	LogTimer += DeltaTime;
	if (LogTimer >= 1.f)
	{
		UE_LOG(LogTemp, Warning, TEXT("Cursor Visible: %s, Cursor Type: %d"), 
			bShowMouseCursor ? TEXT("True") : TEXT("False"), (int32)CurrentMouseCursor.GetValue());
		LogTimer = 0.f;
	}
	UpdateAimRotation();
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
		InputComponent->BindKey(EKeys::I, IE_Pressed, this, &APDPlayerController::ToggleInventory);
		InputComponent->BindKey(EKeys::E, IE_Pressed, this, &APDPlayerController::TryInteract);
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
		InputComponent->BindKey(EKeys::I, IE_Pressed, this, &APDPlayerController::ToggleInventory);
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


	PDIC->BindAbilityActions(InputConfig, this, &APDPlayerController::OnAbilityInputPressed,
		&APDPlayerController::OnAbilityInputReleased);

	PDIC->BindNativeAction(InputConfig, PDGameplayTags::Input_Interact,
		ETriggerEvent::Started, this, &APDPlayerController::OnInteract);
	PDIC->BindNativeAction(InputConfig, PDGameplayTags::Input_SwitchSlot1,
		ETriggerEvent::Started, this, &APDPlayerController::OnSwitchSlot1);
	PDIC->BindNativeAction(InputConfig, PDGameplayTags::Input_SwitchSlot2,
		ETriggerEvent::Started, this, &APDPlayerController::OnSwitchSlot2);
	PDIC->BindNativeAction(InputConfig, PDGameplayTags::Input_SwitchSlot3,
		ETriggerEvent::Started, this, &APDPlayerController::OnSwitchSlot3);
	
	PDIC->BindNativeAction(InputConfig, PDGameplayTags::Input_Zoom,
		ETriggerEvent::Started, this, &APDPlayerController::OnZoom);
	PDIC->BindNativeAction(InputConfig, PDGameplayTags::Input_ToggleFireMode,
		ETriggerEvent::Started, this, &APDPlayerController::OnToggleFireMode);
	PDIC->BindNativeAction(InputConfig, PDGameplayTags::Input_DropWeapon,
		ETriggerEvent::Started, this, &APDPlayerController::OnDropWeapon);

	
}

void APDPlayerController::BeginPlay()
{
	Super::BeginPlay();

	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);

	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;

	CreateAndAddHUDWidget();
}

void APDPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HUDInstance)
	{
		HUDInstance->RemoveFromParent();
		HUDInstance = nullptr;
	}

	if (UPDFrontendUISubsystem* UISubsystem = UPDFrontendUISubsystem::Get(this))
	{
		UISubsystem->CloseScreen();
	}

	Super::EndPlay(EndPlayReason);
}

void APDPlayerController::CreateAndAddHUDWidget()
{
	if (!HUDClass) return;
	if (HUDInstance) return; // 이중 호출 방지

	HUDInstance = CreateWidget<UPDHUDWidget>(this, HUDClass);
	if (HUDInstance)
	{
		HUDInstance->AddToViewport(0); // ZOrder 0 = 가장 아래, 메뉴(10) 밑에 깔림
		HUDInstance->Activate();
	}
}

void APDPlayerController::RequestCloseCurrentScreen()
{
	if (UPDFrontendUISubsystem* UISubsystem = UPDFrontendUISubsystem::Get(this))
	{
		UISubsystem->CloseScreen();
	}
}

void APDPlayerController::OnMove(const struct FInputActionValue& Value)
{
	APawn* ControlledPawn =GetPawn();
	if (!ControlledPawn) return;

	const FVector2D MoveInput=Value.Get<FVector2D>();
	ControlledPawn->AddMovementInput(FVector::ForwardVector, MoveInput.Y);
	ControlledPawn->AddMovementInput(FVector::RightVector, MoveInput.X);
}

void APDPlayerController::OnJump()
{
	if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetPawn()))
	{
		OwnerCharacter->Jump();
	}
}

void APDPlayerController::OnAbilityInputPressed(FGameplayTag InputTag)
{
	if (UAbilitySystemComponent* ASC=UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn()))
	{
		ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(InputTag));
	}
}


void APDPlayerController::OnAbilityInputReleased(FGameplayTag InputTag)
{
	if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn()))
	{
		FGameplayTagContainer TagContainer(InputTag);
		ASC->CancelAbilities(&TagContainer);
	}
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
}

bool APDPlayerController::IsMarketInterfaceOpen() const
{
	return MarketWidgetInstance && MarketWidgetInstance->IsInViewport();
}

void APDPlayerController::OpenStashInterface()
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

	if (IsMarketInterfaceOpen())
	{
		CloseMarketInterface();
	}

	if (!InventoryWidgetInstance || !InventoryWidgetInstance->IsInViewport())
	{
		InventoryWidgetInstance = CreateWidget<UPDInventoryWidget>(this, InventoryWidgetClass);
		if (InventoryWidgetInstance)
		{
			InventoryWidgetInstance->AddToViewport();
		}
	}

	if (!StashWidgetInstance || !StashWidgetInstance->IsInViewport())
	{
		StashWidgetInstance = CreateWidget<UPDStashWidget>(this, StashWidgetClass);
		if (StashWidgetInstance)
		{
			StashWidgetInstance->AddToViewport();
		}
	}
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

void APDPlayerController::ToggleInventory()
{
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

	if (InventoryWidgetInstance && InventoryWidgetInstance->IsInViewport())
	{
		InventoryWidgetInstance->RemoveFromParent();
		InventoryWidgetInstance = nullptr;

		FInputModeGameOnly InputMode;
		SetInputMode(InputMode);
		bShowMouseCursor = false;
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

	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(InventoryWidgetInstance->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	
	SetInputMode(InputMode);
	bShowMouseCursor = true;
}

void APDPlayerController::TryInteract()
{
	if (APDPlayerCharacter* PlayerCharacter = Cast<APDPlayerCharacter>(GetPawn()))
	{
		PlayerCharacter->TryInteract();
	}
}

void APDPlayerController::UpdateAimRotation()
{
	APawn* ControlledPawn =GetPawn();
	if (!ControlledPawn) return;

	FHitResult Hit;
	if (!GetHitResultUnderCursor(ECC_Visibility, true, Hit)) return;

	FVector AimDirection=Hit.Location-ControlledPawn->GetActorLocation();
	AimDirection.Z=0.f;

	if (!AimDirection.IsNearlyZero())
	{
		ControlledPawn->SetActorRotation(AimDirection.Rotation());

		DrawDebugLine(GetWorld(), ControlledPawn->GetActorLocation(),
			ControlledPawn->GetActorLocation() + AimDirection.GetSafeNormal() * 200.f,
			FColor::Blue, false, 0.1f, 0, 2.f);
	}
}

// 슬롯 전환
void APDPlayerController::OnSwitchSlot1()
{
	if (APDPlayerCharacter* Ch = Cast<APDPlayerCharacter>(GetPawn()))
		Ch->SwitchToSlot(EWeaponSlot::Slot1_Rifle);
}

void APDPlayerController::OnSwitchSlot2()
{
	if (APDPlayerCharacter* Ch = Cast<APDPlayerCharacter>(GetPawn()))
		Ch->SwitchToSlot(EWeaponSlot::Slot2_Shotgun);
}

void APDPlayerController::OnSwitchSlot3()
{
	if (APDPlayerCharacter* Ch = Cast<APDPlayerCharacter>(GetPawn()))
		Ch->SwitchToSlot(EWeaponSlot::Slot3_Sniper);
}

void APDPlayerController::OnZoom()
{
	if (APDPlayerCharacter* Ch = Cast<APDPlayerCharacter>(GetPawn()))
		if (APDWeaponBase* Weapon = Ch->GetCurrentWeapon())
			if (APDSniper* Sniper = Cast<APDSniper>(Weapon))
				Sniper->ToggleZoom();
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

void APDPlayerController::OnInteract()
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn) return;

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

