#include "Core/PDPlayerController.h"
#include "Characters/PDPlayerCharacter.h"

#include "AbilitySystemComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Engine/World.h"
#include "Engine/EngineTypes.h"
#include "Net/UnrealNetwork.h"

#include "GameplayTag/PDGameplayTags.h"
#include "InputActionValue.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Core/PDGameMode.h"
#include "Core/PDPlayerState.h"
#include "Core/PDPlayerUIManagerComponent.h"
#include "Engine/LocalPlayer.h"
#include "TimerManager.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Ability/GA_GameplayAbilityBase.h"
#include "Ability/GA_SprintAbility.h"
#include "GameFramework/Character.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Input/PDInputComponent.h"

#include "InputCoreTypes.h"
#include "Widgets/Inventory/PDInventoryWidget.h"
#include "Widgets/Inventory/PDStashWidget.h"
#include "Widgets/Inventory/PDLootWidget.h"
#include "Items/Containers/PDLootComponent.h"
#include "Widgets/Inventory/PDMarketWidget.h"
#include "Widgets/Quest/PDQuestWindowWidget.h"
#include "Items/Market/PDMarketComponent.h"
#include "Items/Containers/PDInventoryComponent.h"
#include "Items/Equipment/PDEquipmentComponent.h"
#include "Items/Containers/PDQuickSlotComponent.h"
#include "Data/PDQuestComponent.h"
#include "Items/Containers/PDStashComponent.h"
#include "Widgets/HUD/PDHUDWidget.h"
#include "Widgets/PDActivatableBase.h"
#include "Widgets/PDRootLayout.h"
#include "Widgets/PDNotificationWidget.h"
#include "Widgets/Transition/PDRaidEndTransitionWidget.h"
#include "Widgets/Transition/PDRaidStartTransitionWidget.h"
#include "Game/PDRaidStats.h"
#include "Subsystems/PDFrontendUISubsystem.h"
<<<<<<< HEAD
#include "Subsystems/PDLoadingScreenSubsystem.h"
#include "Subsystems/PDQuipSubsystem.h"
#include "Data/PDQuipDataAsset.h"
=======
>>>>>>> origin/main
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

void APDPlayerController::Server_RequestBaseTravel_Implementation()
{
	if (APDGameMode* GM = GetWorld()->GetAuthGameMode<APDGameMode>())
	{
		GM->NotifyPlayerReadyForTravel(this);
	}
}

void APDPlayerController::Client_ShowRaidEndTransition_Implementation(bool bSuccess,
                                                                       const TArray<FPDPlayerRaidEntryData>& Entries,
                                                                       float RaidDurationSeconds)
{
	if (!RaidEndTransitionClass)
	{
		UE_LOG(LogPDCharacter, Warning, TEXT("Client_ShowRaidEndTransition: RaidEndTransitionClass 미할??(PC=%s)"), *GetName());
		return;
	}

	UPDFrontendUISubsystem* Subsystem = UPDFrontendUISubsystem::Get(this);
	if (!Subsystem)
	{
		UE_LOG(LogPDCharacter, Warning, TEXT("Client_ShowRaidEndTransition: FrontendUISubsystem null (PC=%s)"), *GetName());
		return;
	}

	UPDActivatableBase* Pushed = Subsystem->PushToLayer(EUILayer::Modal, RaidEndTransitionClass);
	UPDRaidEndTransitionWidget* Transition = Cast<UPDRaidEndTransitionWidget>(Pushed);
	if (!Transition)
	{
		UE_LOG(LogPDCharacter, Warning, TEXT("Client_ShowRaidEndTransition: PushToLayer ?�패 ?�는 캐스???�패 (PC=%s)"), *GetName());
		return;
	}

	Transition->Configure(bSuccess, Entries, RaidDurationSeconds);
}

void APDPlayerController::Client_ShowRaidStartTransition_Implementation(const FText& ZoneName)
{
	// PostLogin 시점 push 라 RootLayout 등록 전에 도착할 수 있음 → 준비될 때까지 재시도.
	PendingRaidStartZoneName = ZoneName;
	bHasPendingRaidStartTransition = true;
	RaidStartTransitionRetryCount = 0;
	TryShowRaidStartTransition();
}

void APDPlayerController::TryShowRaidStartTransition()
{
	if (!bHasPendingRaidStartTransition) return;

	if (!RaidStartTransitionClass)
	{
		UE_LOG(LogPDCharacter, Warning, TEXT("RaidStartTransition: RaidStartTransitionClass 미할당 (PC=%s)"), *GetName());
		bHasPendingRaidStartTransition = false;
		return;
	}

	UPDFrontendUISubsystem* Subsystem = UPDFrontendUISubsystem::Get(this);
	const bool bUIReady = Subsystem && Subsystem->GetRootLayout();

	// RootLayout 준비되면 즉시 push — 로딩스크린이 떠 있어도 OK (위젯이 검정을 그 밑에 미리 깔고,
	// 로딩스크린 내려가는 순간 조립 시작하므로 raw 맵 노출 갭이 없음). 로딩스크린 대기는 위젯이 담당.
	if (!bUIReady)
	{
		if (++RaidStartTransitionRetryCount > 50)
		{
			UE_LOG(LogPDCharacter, Warning, TEXT("RaidStartTransition: UI 준비 안 됨, 재시도 포기 (PC=%s)"), *GetName());
			bHasPendingRaidStartTransition = false;
			return;
		}
		GetWorldTimerManager().SetTimer(
			RaidStartTransitionRetryHandle, this, &APDPlayerController::TryShowRaidStartTransition, 0.1f, false);
		return;
	}

	UPDActivatableBase* Pushed = Subsystem->PushToLayer(EUILayer::Modal, RaidStartTransitionClass);
	UPDRaidStartTransitionWidget* Transition = Cast<UPDRaidStartTransitionWidget>(Pushed);
	if (!Transition)
	{
		UE_LOG(LogPDCharacter, Warning, TEXT("RaidStartTransition: PushToLayer 실패 또는 캐스트 실패 (PC=%s)"), *GetName());
		bHasPendingRaidStartTransition = false;
		return;
	}

	Transition->Configure(PendingRaidStartZoneName);
	bHasPendingRaidStartTransition = false;
}

void APDPlayerController::Client_NotifyReviveStarted_Implementation(AActor* Target, float Duration)
{
	if (UIManagerComponent)
	{
		if (UPDHUDWidget* HUD = UIManagerComponent->GetHUDInstance())
		{
			HUD->StartReviveProgress(Target, Duration);
		}
	}
}

void APDPlayerController::Client_NotifyReviveEnded_Implementation(bool bCompleted)
{
	if (UIManagerComponent)
	{
		if (UPDHUDWidget* HUD = UIManagerComponent->GetHUDInstance())
		{
			if (bCompleted) HUD->CompleteReviveProgress();
			else            HUD->StopReviveProgress();
		}
	}
}

void APDPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(APDPlayerController, bIsSpectating, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(APDPlayerController, SpectateTargetPC, COND_OwnerOnly);
}

void APDPlayerController::StartSpectatingDeath(APlayerController* InitialTarget)
{
	if (!HasAuthority()) return;
	if (!InitialTarget || !InitialTarget->GetPawn()) return;
	if (InitialTarget == this) return;

	bIsSpectating = true;
	SpectateTargetPC = InitialTarget;
	SetViewTargetWithBlend(InitialTarget->GetPawn(), 0.5f);

	// Listen-server host(?�버=로컬)?�서??OnRep ???�동 발화?��? ?�으므�??�동 브로?�캐?�트.
	if (IsLocalController())
	{
		OnRep_SpectateState();
	}

	UE_LOG(LogPDCharacter, Log, TEXT("StartSpectatingDeath: %s -> %s"),
		*GetName(), *InitialTarget->GetName());
}

void APDPlayerController::CycleSpectateIfTargetIs(APlayerController* AffectedPC)
{
	if (!HasAuthority()) return;
	if (!bIsSpectating) return;
	if (SpectateTargetPC != AffectedPC) return;
	CycleSpectateTargetServer(+1);
}

void APDPlayerController::CycleSpectateTargetServer(int32 Direction)
{
	if (!HasAuthority()) return;
	UWorld* World = GetWorld();
	if (!World) return;

	TArray<APlayerController*> Candidates;
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!PC || PC == this) continue;

		APDPlayerState* PS = PC->GetPlayerState<APDPlayerState>();
		if (!PS) continue;
		if (PS->IsRaidDead() || PS->IsExtracted()) continue;
		if (!PC->GetPawn()) continue;

		Candidates.Add(PC);
	}

	if (Candidates.Num() == 0)
	{
		// ???�상 ?�아?�는 관???�?�이 ?�음. ?�기 ?�신 ?�점?�로 ?�냅?�면 죽�? ?�기 몸으�?컷이 ?�??		// 부?�연?�러?��?�?마�?막으�?보던 ??방금 죽거??추출???�?????�점??그�?�??��?.
		// EndRaid 가 �?결산 ?�젯???�우므�?카메???�태??거기????��.
		UE_LOG(LogPDCharacter, Log, TEXT("CycleSpectate: %s -> no candidates, keep current view target"), *GetName());
		return;
	}

	const int32 CurrentIdx = Candidates.IndexOfByKey(SpectateTargetPC.Get());
	int32 NextIdx;
	if (CurrentIdx == INDEX_NONE)
	{
		NextIdx = (Direction > 0) ? 0 : (Candidates.Num() - 1);
	}
	else
	{
		NextIdx = (CurrentIdx + Direction + Candidates.Num()) % Candidates.Num();
	}

	SpectateTargetPC = Candidates[NextIdx];
	SetViewTargetWithBlend(SpectateTargetPC->GetPawn(), 0.3f);
	if (IsLocalController())
	{
		OnRep_SpectateState();
	}

	UE_LOG(LogPDCharacter, Log, TEXT("CycleSpectate: %s -> %s (idx=%d/%d, dir=%d)"),
		*GetName(), *SpectateTargetPC->GetName(), NextIdx, Candidates.Num(), Direction);
}

void APDPlayerController::Server_SpectateNext_Implementation()
{
	if (!bIsSpectating) return;
	CycleSpectateTargetServer(+1);
}

void APDPlayerController::Server_SpectatePrev_Implementation()
{
	if (!bIsSpectating) return;
	CycleSpectateTargetServer(-1);
}

void APDPlayerController::OnSpectateNextInput()
{
	if (!bIsSpectating) return;
	Server_SpectateNext();
}

void APDPlayerController::OnSpectatePrevInput()
{
	if (!bIsSpectating) return;
	Server_SpectatePrev();
}

void APDPlayerController::OnRep_SpectateState()
{
	OnSpectateChanged.Broadcast();
}

FString APDPlayerController::GetSpectateTargetName() const
{
	if (SpectateTargetPC)
	{
		if (APlayerState* PS = SpectateTargetPC->PlayerState)
		{
			return PS->GetPlayerName();
		}
	}
	return FString();
}

APDPlayerState* APDPlayerController::GetPDPlayerState() const
{
	return GetPlayerState<APDPlayerState>();
}

UPDInventoryComponent* APDPlayerController::GetPlayerInventoryComponent() const
{
	UPDInventoryComponent* EditorInventoryComponent = GetPawn() ? GetPawn()->FindComponentByClass<UPDInventoryComponent>() : nullptr;

	if (APDPlayerState* PDPlayerState = GetPDPlayerState())
	{
		UPDInventoryComponent* RuntimeInventoryComponent = PDPlayerState->GetInventoryComponent();
		if (RuntimeInventoryComponent && EditorInventoryComponent && RuntimeInventoryComponent != EditorInventoryComponent)
		{
			const int32 EditorGridColumns = FMath::Max(1, EditorInventoryComponent->GridColumns);
			const int32 EditorGridRows = FMath::Max(1, EditorInventoryComponent->GridRows);
			const bool bGridChanged = RuntimeInventoryComponent->GridColumns != EditorGridColumns || RuntimeInventoryComponent->GridRows != EditorGridRows;

			RuntimeInventoryComponent->GridColumns = EditorGridColumns;
			RuntimeInventoryComponent->GridRows = EditorGridRows;
			RuntimeInventoryComponent->BaseCarryWeight = EditorInventoryComponent->BaseCarryWeight;

			if (EditorInventoryComponent->ItemDataTable)
			{
				RuntimeInventoryComponent->ItemDataTable = EditorInventoryComponent->ItemDataTable;
			}

			if (bGridChanged && RuntimeInventoryComponent->GetOwner() && RuntimeInventoryComponent->GetOwner()->HasAuthority())
			{
				RuntimeInventoryComponent->InitializeInventory();
			}
		}

		return RuntimeInventoryComponent;
	}

	return EditorInventoryComponent;
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
<<<<<<< HEAD
	TickRecoilRecovery(DeltaTime);
=======
>>>>>>> origin/main
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
		InputComponent->BindKey(EKeys::Tab, IE_Pressed, this, &APDPlayerController::ToggleHub);
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
			ETriggerEvent::Started, this, &APDPlayerController::ToggleHub);
	}
	else
	{
		InputComponent->BindKey(EKeys::Tab, IE_Pressed, this, &APDPlayerController::ToggleHub);
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

	// Step 2-B: 관???�음/?�전 (?�망 ?�에�??�성?????�들?�에??bIsSpectating 체크).
	if (InputConfig->FindNativeInputActionForTag(PDGameplayTags::Input_SpectateNext))
	{
		PDIC->BindNativeAction(InputConfig, PDGameplayTags::Input_SpectateNext,
			ETriggerEvent::Started, this, &APDPlayerController::OnSpectateNextInput);
	}
	else
	{
		InputComponent->BindKey(EKeys::F, IE_Pressed, this, &APDPlayerController::OnSpectateNextInput);
	}

	if (InputConfig->FindNativeInputActionForTag(PDGameplayTags::Input_SpectatePrev))
	{
		PDIC->BindNativeAction(InputConfig, PDGameplayTags::Input_SpectatePrev,
			ETriggerEvent::Started, this, &APDPlayerController::OnSpectatePrevInput);
	}
	else
	{
		InputComponent->BindKey(EKeys::G, IE_Pressed, this, &APDPlayerController::OnSpectatePrevInput);
	}
}

void APDPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// 멀?? ?�력모드/HUD/RootLayout/Subsystem ?�록?� 로컬 PC?�서�??��?가 ?�음.
	// ?�버???�는 ?�격 ?�라?�언?�의 PC ?�스?�스가 ?�기 ?�어?�면 GI ?�일 RootLayout??마�?�??�출????��?�는 ?�고가 ??
	if (!IsLocalController())
	{
		return;
	}

	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);

<<<<<<< HEAD
	bShowMouseCursor = false;
=======
	bShowMouseCursor = true;
>>>>>>> origin/main
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
	DefaultMouseCursor = EMouseCursor::Default;
	CurrentMouseCursor = EMouseCursor::Default;

	if (APDPlayerCharacter* Ch = Cast<APDPlayerCharacter>(GetPawn()))
	{
		Ch->OnWeaponSwapped.AddDynamic(this, &APDPlayerController::OnWeaponChanged);
	}

	if (UPDQuipSubsystem* QuipSub = UPDQuipSubsystem::Get(this))
	{
		if (UPDQuipDataAsset* LoadedDA = QuipDataAsset.LoadSynchronous())
		{
			QuipSub->SetQuipDataAsset(LoadedDA);
		}
		QuipSub->NotifyPawnChanged(GetPawn());
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

void APDPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	BindInventoryNotifications();
}

void APDPlayerController::PreClientTravel(const FString& PendingURL, ETravelType TravelType, bool bIsSeamlessTravel)
{
	Super::PreClientTravel(PendingURL, TravelType, bIsSeamlessTravel);

	// ?�라?�언??머신??PC�???hook?�로 LoadingScreen???��?.
	// ?�스??=Authority)???�기 ?�래�???TravelToLevel?�서 ?��? ShowImmediate ?�출?��?�??�외 ??	// 그렇지 ?�으�????�라?�언??join처럼 ?�스??측에 PreClientTravel???�떤 경로로든 trigger????중복 ?�시??
	if (!IsLocalController() || HasAuthority())
	{
		return;
	}

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UPDLoadingScreenSubsystem* LSS = GI->GetSubsystem<UPDLoadingScreenSubsystem>())
		{
			LSS->ShowImmediate();
		}
	}
}

void APDPlayerController::OnPossess(APawn* InPawn)
{
	UnbindInventoryNotifications();
	Super::OnPossess(InPawn);

	// Step 2-B ?�속: ????빙의 ??관???�태 carry-over 차단. HUD "Spectating: ..." ?�존 방�?.
	if (HasAuthority() && (bIsSpectating || SpectateTargetPC))
	{
		bIsSpectating = false;
		SpectateTargetPC = nullptr;
		if (IsLocalController())
		{
			OnRep_SpectateState();
		}
	}

	if (UIManagerComponent)
	{
		UAbilitySystemComponent* ASC = InPawn
			? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(InPawn)
			: nullptr;
		UIManagerComponent->RebindHUDToASC(ASC);
		UIManagerComponent->RefreshNewQuickSlots();
	}

	if (IsLocalController())
	{
		if (UPDQuipSubsystem* QuipSub = UPDQuipSubsystem::Get(this))
		{
			QuipSub->NotifyPawnChanged(InPawn);
		}
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

	if (IsLocalController())
	{
		if (UPDQuipSubsystem* QuipSub = UPDQuipSubsystem::Get(this))
		{
			QuipSub->NotifyPawnChanged(nullptr);
		}
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
<<<<<<< HEAD
	if (IsGameplayInputBlockedByModalUI() && !ShouldAllowMovementWhileUIOpen()) return;
=======
	if (IsGameplayInputBlockedByModalUI()) return;

	APawn* ControlledPawn =GetPawn();
	if (!ControlledPawn) return;
>>>>>>> origin/main

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

<<<<<<< HEAD
	if (const APDPlayerCharacter* PlayerCharacter = Cast<APDPlayerCharacter>(GetPawn()))
	{
		if (PlayerCharacter->IsDowned() || PlayerCharacter->IsGettingUp() || PlayerCharacter->IsDead()) return;
	}

=======
>>>>>>> origin/main
	if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetPawn()))
	{
		OwnerCharacter->Jump();
	}
}

void APDPlayerController::OnAbilityInputPressed(FGameplayTag InputTag)
{
	if (InputTag == PDGameplayTags::Input_Interact)
	{
		return;
	}
	if (const APDPlayerCharacter* PlayerCharacter = Cast<APDPlayerCharacter>(GetPawn()))
	{
		if (PlayerCharacter->IsDowned() || PlayerCharacter->IsGettingUp() || PlayerCharacter->IsDead()) return;
	}

	if (UAbilitySystemComponent* ASC=UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn()))
	{
		if (InputTag == PDGameplayTags::Input_Roll ||
			InputTag == PDGameplayTags::Input_Sprint ||
			InputTag == PDGameplayTags::Input_Bombing)
		{
			if (InputTag == PDGameplayTags::Input_Sprint &&
				ASC->HasMatchingGameplayTag(PDGameplayTags::State_Sprinting))
			{
				return;
			}

			const bool bActivated = ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(InputTag));
			if (!HasAuthority() && !bActivated)
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
			Spec.Ability->IsA<UGA_SprintAbility>();

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
	if (InputTag == PDGameplayTags::Input_Interact)
	{
		return;
	}
	if (const APDPlayerCharacter* PlayerCharacter = Cast<APDPlayerCharacter>(GetPawn()))
	{
		if (PlayerCharacter->IsDowned() || PlayerCharacter->IsGettingUp() || PlayerCharacter->IsDead()) return;
	}

	if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn()))
	{
		if (InputTag == PDGameplayTags::Input_Sprint &&
			ASC->HasMatchingGameplayTag(PDGameplayTags::State_Sprinting))
		{
			return;
		}

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
<<<<<<< HEAD
	UIManagerComponent->OpenMarket(MarketComponent);
=======

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

	SetGameplayInputBlockedByModalUI(true, MarketWidgetInstance);
>>>>>>> origin/main
}

void APDPlayerController::CloseMarketInterface()
{
	if (UIManagerComponent)
	{
		UIManagerComponent->CloseMarket();
	}
<<<<<<< HEAD
=======
	MarketWidgetInstance = nullptr;
	ActiveMarketComponent = nullptr;

	if (InventoryWidgetInstance && InventoryWidgetInstance->IsInViewport())
	{
		InventoryWidgetInstance->RemoveFromParent();
	}
	InventoryWidgetInstance = nullptr;

	if (!IsStashInterfaceOpen())
	{
		SetGameplayInputBlockedByModalUI(false);
	}
>>>>>>> origin/main
}

bool APDPlayerController::IsMarketInterfaceOpen() const
{
	return UIManagerComponent && UIManagerComponent->IsMarketOpen();
}

UPDMarketComponent* APDPlayerController::GetActiveMarketComponent() const
{
	return UIManagerComponent ? UIManagerComponent->GetActiveMarketComponent() : nullptr;
}

void APDPlayerController::OpenStashInterface(UPDStashComponent* StashSource)
{
	if (!StashSource)
	{
		return;
	}

	if (HasAuthority())
	{
		StashSource->LoadFromPlayerState(GetPlayerState<APDPlayerState>());

		if (IsLocalController())
		{
			if (UIManagerComponent)
			{
				UIManagerComponent->OpenStash(StashSource);
			}
		}
		else
		{
			ClientOpenStashInterface(StashSource);
		}
		return;
	}

	if (UIManagerComponent)
	{
		UIManagerComponent->OpenStash(StashSource);
	}

	SetGameplayInputBlockedByModalUI(true, StashWidgetInstance);
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

void APDPlayerController::ClientCloseStashInterface_Implementation(UPDStashComponent* StashSource)
{
	if (!UIManagerComponent) return;
	if (!StashSource || UIManagerComponent->GetActiveStashComponent() == StashSource)
	{
		UIManagerComponent->CloseStash();
	}
}
void APDPlayerController::CloseStashInterface()
{
	UPDStashComponent* ClosingStash = GetActiveStashComponent();

	if (UIManagerComponent)
	{
		UIManagerComponent->CloseStash();
	}

	if (!HasAuthority() && ClosingStash)
	{
		ServerNotifyStashInterfaceClosed(ClosingStash);
	}
}

void APDPlayerController::ServerNotifyStashInterfaceClosed_Implementation(UPDStashComponent* StashSource)
{
	if (StashSource)
	{
		OnStashInterfaceClosed.Broadcast(StashSource);
	}
<<<<<<< HEAD
=======
	InventoryWidgetInstance = nullptr;

	if (!IsMarketInterfaceOpen())
	{
		SetGameplayInputBlockedByModalUI(false);
	}
>>>>>>> origin/main
}

bool APDPlayerController::IsStashInterfaceOpen() const
{
	return UIManagerComponent && UIManagerComponent->IsStashOpen();
}


UPDStashComponent* APDPlayerController::GetActiveStashComponent() const
{
	return UIManagerComponent ? UIManagerComponent->GetActiveStashComponent() : nullptr;
}

// ?�?�?� LootBox ?�터?�이???�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�
// codex 브랜치는 InventoryWidget ?�이?�사?�클??UIManager �??�임?��?�?develop 구현???�순????// LootWidget �?PlayerController 가 직접 ?�루�? ?�력 차단?� 기존 ?�퍼 ?�사??
void APDPlayerController::OpenLootInterface(UPDLootComponent* LootSource)
{
	if (!LootWidgetClass || !LootSource)
	{
		UE_LOG(LogPDCharacter, Warning, TEXT("OpenLootInterface: missing widget class or source"));
		return;
	}

	// 위젯 생성은 로컬 PC 에서만 가능 — 서버에서 원격 클라 대상이면 Client RPC 로 위임(Stash/Market 동일 패턴).
	if (HasAuthority() && !IsLocalController())
	{
		ClientOpenLootInterface(LootSource);
		return;
	}

	OpenLootInterfaceLocal(LootSource);
}

void APDPlayerController::ClientOpenLootInterface_Implementation(UPDLootComponent* LootSource)
{
	// 토글: 같은 박스 재상호작용이면 닫기. 서버는 위젯 상태를 모르므로 토글 판정은 클라에서.
	if (IsLootInterfaceOpen() && GetActiveLootComponent() == LootSource)
	{
		CloseLootInterfaceLocal();
		return;
	}
	OpenLootInterfaceLocal(LootSource);
}

void APDPlayerController::OpenLootInterfaceLocal(UPDLootComponent* LootSource)
{
	if (!LootWidgetClass || !LootSource)
	{
		UE_LOG(LogPDCharacter, Warning, TEXT("OpenLootInterface: missing widget class or source"));
		return;
	}

	// ?�른 모달 UI 가 같�? viewport ?�역???�유?��?�?먼�? ?�음.
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

	// Stash/Market 처럼 InventoryWidget ??같이 ?��? ??Loot ?� ?�벤 �??�래�?비교 가??
	if (UIManagerComponent)
	{
		UIManagerComponent->OpenInventoryForLoot();
	}

	SetGameplayInputBlockedByModalUI(true, LootWidgetInstance);
}

void APDPlayerController::CloseLootInterface()
{
	// 위젯 제거도 로컬 PC 에서만 — 서버에서 원격 클라 대상이면 Client RPC 로 위임.
	if (HasAuthority() && !IsLocalController())
	{
		ClientCloseLootInterface(ActiveLootComponent.Get());
		return;
	}

	CloseLootInterfaceLocal();
}

void APDPlayerController::ClientCloseLootInterface_Implementation(UPDLootComponent* LootSource)
{
	if (!LootSource || GetActiveLootComponent() == LootSource)
	{
		CloseLootInterfaceLocal();
	}
}

void APDPlayerController::CloseLootInterfaceLocal()
{
	UPDLootComponent* ClosingLootComponent = ActiveLootComponent.Get();

	if (LootWidgetInstance && LootWidgetInstance->IsInViewport())
	{
		LootWidgetInstance->RemoveFromParent();
	}
	LootWidgetInstance = nullptr;
	ActiveLootComponent.Reset();

	// Loot ?�젯�??�께 ?�웠??InventoryWidget ???�제 (?�른 UI 가 ?��? ??????.
	if (UIManagerComponent)
	{
		UIManagerComponent->CloseInventoryForLoot();
	}

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

void APDPlayerController::ServerTakeLootSlotToInventory_Implementation(UPDLootComponent* LootComponent, int32 LootSlotIndex, int32 Quantity)
{
	if (!LootComponent) return;
	if (UPDInventoryComponent* InventoryComponent = GetPlayerInventoryComponent())
	{
		LootComponent->TakeSlotToInventory(LootSlotIndex, InventoryComponent, Quantity);
	}
}

void APDPlayerController::ServerTakeLootSlotQuantityToInventorySlot_Implementation(UPDLootComponent* LootComponent, int32 LootSlotIndex, int32 TargetInventorySlotIndex, int32 Quantity)
{
	if (!LootComponent) return;
	if (UPDInventoryComponent* InventoryComponent = GetPlayerInventoryComponent())
	{
		LootComponent->TakeSlotQuantityToInventorySlot(InventoryComponent, LootSlotIndex, TargetInventorySlotIndex, Quantity);
	}
}

void APDPlayerController::ServerStoreInventorySlotQuantityToLoot_Implementation(UPDLootComponent* LootComponent, int32 SourceSlotIndex, int32 TargetLootSlotIndex, int32 Quantity)
{
	if (!LootComponent) return;
	if (UPDInventoryComponent* InventoryComponent = GetPlayerInventoryComponent())
	{
		LootComponent->StoreInventorySlotQuantityToSlot(InventoryComponent, SourceSlotIndex, TargetLootSlotIndex, Quantity);
	}
}

void APDPlayerController::ServerMoveLootSlotQuantity_Implementation(UPDLootComponent* LootComponent, int32 SourceSlotIndex, int32 TargetSlotIndex, int32 Quantity)
{
	if (LootComponent)
	{
		LootComponent->MoveSlotQuantityToSlot(SourceSlotIndex, TargetSlotIndex, Quantity);
	}
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

<<<<<<< HEAD
void APDPlayerController::ServerUpgradeStash_Implementation(UPDStashComponent* StashComponent)
{
	if (!StashComponent)
	{
=======
		bIsGameplayInputBlockedByModalUI = false;
		SetIgnoreMoveInput(false);
		SetIgnoreLookInput(false);

		FInputModeGameOnly InputMode;
		SetInputMode(InputMode);
		bShowMouseCursor = false;
>>>>>>> origin/main
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

<<<<<<< HEAD
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

void APDPlayerController::ServerMoveInventorySlotQuantity_Implementation(int32 SourceSlotIndex, int32 TargetSlotIndex, int32 Quantity)
{
	if (UPDInventoryComponent* InventoryComponent = GetPlayerInventoryComponent())
	{
		InventoryComponent->MoveSlotQuantityToSlot(SourceSlotIndex, TargetSlotIndex, Quantity);
	}
}

void APDPlayerController::ServerStoreInventorySlotQuantityToQuickSlot_Implementation(int32 SourceSlotIndex, int32 TargetQuickSlotIndex, int32 Quantity)
{
	UPDInventoryComponent* InventoryComponent = GetPlayerInventoryComponent();
	UPDQuickSlotComponent* QuickSlotComponent = GetPlayerQuickSlotComponent();
	if (InventoryComponent && QuickSlotComponent)
	{
		QuickSlotComponent->StoreInventorySlotQuantityToSlot(InventoryComponent, SourceSlotIndex, TargetQuickSlotIndex, Quantity);
	}
}

void APDPlayerController::ServerMoveQuickSlotQuantity_Implementation(int32 SourceSlotIndex, int32 TargetSlotIndex, int32 Quantity)
{
	if (UPDQuickSlotComponent* QuickSlotComponent = GetPlayerQuickSlotComponent())
	{
		QuickSlotComponent->MoveSlotQuantityToSlot(SourceSlotIndex, TargetSlotIndex, Quantity);
	}
}

void APDPlayerController::ServerEquipInventoryWeaponSlot_Implementation(int32 InventorySlotIndex)
{
	if (UPDQuickSlotComponent* QuickSlotComponent = GetPlayerQuickSlotComponent())
	{
		QuickSlotComponent->EquipInventoryWeaponSlot(InventorySlotIndex);
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
	BoundInventoryNotificationComponent->OnInventoryWeightLimitExceeded.AddUniqueDynamic(this, &APDPlayerController::HandleInventoryWeightLimitExceeded);
}

void APDPlayerController::UnbindInventoryNotifications()
{
	if (BoundInventoryNotificationComponent)
	{
		BoundInventoryNotificationComponent->OnInventoryMessage.RemoveDynamic(this, &APDPlayerController::HandleInventoryMessage);
		BoundInventoryNotificationComponent->OnInventoryWeightLimitExceeded.RemoveDynamic(this, &APDPlayerController::HandleInventoryWeightLimitExceeded);
		BoundInventoryNotificationComponent = nullptr;
	}
}

void APDPlayerController::HandleInventoryMessage(const FText& Message)
{
	if (Message.EqualTo(FText::FromString(TEXT("Weight limit exceeded."))))
	{
		return;
	}

	ShowNotification(Message);
}

void APDPlayerController::HandleInventoryWeightLimitExceeded(float CurrentWeight, float MaxWeight)
{
	const FText Message = FText::Format(
		NSLOCTEXT("PDInventory", "WeightLimitExceededFormat", "Weight limit exceeded. {0} / {1}kg"),
		FText::AsNumber(CurrentWeight),
		FText::AsNumber(MaxWeight));
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

void APDPlayerController::ToggleHub()
{
	if (UIManagerComponent)
	{
		UIManagerComponent->ToggleHub();
	}
}

TArray<FKey> APDPlayerController::GetMappedKeysForInputTag(const FGameplayTag& InputTag) const
{
	TArray<FKey> Keys;
	if (!InputConfig || !DefaultMappingContext)
	{
		return Keys;
	}

	const UInputAction* Action = InputConfig->FindNativeInputActionForTag(InputTag);
	if (!Action)
	{
		return Keys;
	}

	for (const FEnhancedActionKeyMapping& Mapping : DefaultMappingContext->GetMappings())
	{
		if (Mapping.Action == Action)
		{
			Keys.AddUnique(Mapping.Key);
		}
	}
	return Keys;
=======
	bIsGameplayInputBlockedByModalUI = true;
	SetIgnoreMoveInput(true);
	SetIgnoreLookInput(true);

	if (APawn* ControlledPawn = GetPawn())
	{
		if (UPawnMovementComponent* MovementComponent = ControlledPawn->GetMovementComponent())
		{
			MovementComponent->StopMovementImmediately();
		}
	}

	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(InventoryWidgetInstance->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);

	SetInputMode(InputMode);
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
	DefaultMouseCursor = EMouseCursor::Default;
	CurrentMouseCursor = EMouseCursor::Default;
>>>>>>> origin/main
}

void APDPlayerController::TryInteract()
{
	if (APDPlayerCharacter* PlayerCharacter = Cast<APDPlayerCharacter>(GetPawn()))
	{
		PlayerCharacter->TryInteract();
	}
}

<<<<<<< HEAD
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
=======

bool APDPlayerController::IsGameplayInputBlockedByModalUI() const
{
	return bIsGameplayInputBlockedByModalUI;
>>>>>>> origin/main
}

void APDPlayerController::SetGameplayInputBlockedByModalUI(bool bBlocked, UUserWidget* WidgetToFocus)
{
<<<<<<< HEAD
	if (UIManagerComponent)
	{
		UIManagerComponent->SetGameplayInputBlockedByModalUI(bBlocked, WidgetToFocus);
	}
}
=======
	if (bBlocked)
	{
		if (!bIsGameplayInputBlockedByModalUI)
		{
			bMouseCursorVisibleBeforeModalUI = bShowMouseCursor;
			bMouseClickEventsEnabledBeforeModalUI = bEnableClickEvents;
			bMouseOverEventsEnabledBeforeModalUI = bEnableMouseOverEvents;
		}

		bIsGameplayInputBlockedByModalUI = true;
		SetIgnoreMoveInput(true);
		SetIgnoreLookInput(true);

		if (APawn* ControlledPawn = GetPawn())
		{
			if (UPawnMovementComponent* MovementComponent = ControlledPawn->GetMovementComponent())
			{
				MovementComponent->StopMovementImmediately();
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
}


>>>>>>> origin/main
void APDPlayerController::UpdateAimRotation()
{
	if (IsGameplayInputBlockedByModalUI()) return;

<<<<<<< HEAD
	if (UAbilitySystemComponent* ASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn()))
	{
		if (ASC->HasMatchingGameplayTag(PDGameplayTags::State_Rolling)) return;
	}

=======
>>>>>>> origin/main
	APawn* ControlledPawn =GetPawn();
	if (!ControlledPawn) return;
	if (const APDPlayerCharacter* PlayerCharacter = Cast<APDPlayerCharacter>(ControlledPawn))
	{
		if (PlayerCharacter->IsDowned() || PlayerCharacter->IsGettingUp() || PlayerCharacter->IsDead()) return;
	}

	FHitResult VisibilityAimHit;
	if (!GetHitResultUnderCursor(ECC_Visibility, true, VisibilityAimHit)) return;

	const TArray<TEnumAsByte<EObjectTypeQuery>> PawnObjectTypes = { UEngineTypes::ConvertToObjectType(ECC_Pawn) };
	FHitResult PawnAimHit;
	const bool bPawnAimHit = GetHitResultUnderCursorForObjects(PawnObjectTypes, true, PawnAimHit)
		&& PawnAimHit.GetActor()
		&& PawnAimHit.GetActor() != ControlledPawn;

	FHitResult RecoiledVisibilityHit;
	if (!GetRecoiledHitResult(ECC_Visibility, true, RecoiledVisibilityHit))
	{
		RecoiledVisibilityHit = VisibilityAimHit;
	}

	FHitResult RecoiledPawnHit;
	const bool bRecoiledPawnHit = GetRecoiledHitResultForObjects(PawnObjectTypes, true, RecoiledPawnHit)
		&& RecoiledPawnHit.GetActor()
		&& RecoiledPawnHit.GetActor() != ControlledPawn;

	const FHitResult& AimHit = bPawnAimHit ? PawnAimHit : VisibilityAimHit;
	const FHitResult& FireAimHit = bRecoiledPawnHit ? RecoiledPawnHit : RecoiledVisibilityHit;


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
		if (PlayerCharacter->IsDowned() || PlayerCharacter->IsGettingUp() || PlayerCharacter->IsDead())
		{
			return;
		}
	}

	if (UPDQuickSlotComponent* QuickSlotComponent = GetPlayerQuickSlotComponent())
	{
		const bool bUsed = QuickSlotComponent->UseQuickSlot(SlotIndex);
		if (bUsed && UIManagerComponent)
		{
			UIManagerComponent->RefreshNewQuickSlots();
		}
		return;
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
		if (PlayerCharacter->IsDowned() || PlayerCharacter->IsGettingUp() || PlayerCharacter->IsDead())
		{
			return;
		}
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

void APDPlayerController::ClientAddRecoilOffset_Implementation(float YawDelta)
{
	AddRecoilOffset(YawDelta);
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
