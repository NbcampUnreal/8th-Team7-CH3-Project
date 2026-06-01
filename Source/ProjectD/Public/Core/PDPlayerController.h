#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "GameplayTagContainer.h"
#include "GameFramework/PlayerController.h"
#include "Type/Types.h"
#include "PDPlayerController.generated.h"

struct FGameplayTag;
class UNiagaraSystem;
class UInputMappingContext;
class UInputAction;
class UPathFollowingComponent;
class UPDInputConfig;
class UPDInventoryWidget;
class UPDStashWidget;
class UPDStashComponent;
class UPDLootWidget;
class UPDLootComponent;
class UPDMarketWidget;
class UPDMarketComponent;
class UPDQuestWindowWidget;
class APDPlayerCharacter;
class UPDHUDWidget;
class UPDActivatableBase;
class UUserWidget;
<<<<<<< HEAD
class UPDNotificationWidget;
class UPDInventoryComponent;
class UPDEquipmentComponent;
class UPDQuickSlotComponent;
class UPDQuestComponent;
class UPDPlayerUIManagerComponent;
class APDPlayerState;
class APDWeaponBase;
class APDRifle;
class UPDRootLayout;
class UPDPingInputComponent;
class UPDWorldMapWidget;
class UPDQuipDataAsset;
class UPDRaidEndTransitionWidget;
class UPDRaidStartTransitionWidget;
struct FPDPlayerRaidEntryData;
enum class EWidgetInputMode : uint8;
=======
>>>>>>> origin/main

DECLARE_LOG_CATEGORY_EXTERN(LogPDCharacter, Log, All);
DECLARE_MULTICAST_DELEGATE_OneParam(FPDLootInterfaceClosedSignature, UPDLootComponent*);
// === Source legacy: ?�터?�이???�힘 ?�리게이???�그?�처 ===
DECLARE_MULTICAST_DELEGATE_OneParam(FPDStashInterfaceClosedSignature, UPDStashComponent*);
DECLARE_MULTICAST_DELEGATE_OneParam(FPDMarketInterfaceClosedSignature, UPDMarketComponent*);
DECLARE_MULTICAST_DELEGATE(FPDEquipmentModificationInterfaceClosedSignature);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPDSpectateChangedDelegate);

UCLASS(abstract)
class PROJECTD_API APDPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	APDPlayerController();

	UFUNCTION(BlueprintCallable, Category = "PD|Raid")
	void RequestExtraction();

	UFUNCTION(Server, Reliable)
	void ServerRequestExtraction();

	// 결산 ?�젯??Anim_BlackFade 종료 ?�점???�출. ?�버?�서 GameMode??게이?�에 ACK ?�록.
	UFUNCTION(Server, Reliable)
	void Server_RequestBaseTravel();

	// ?�버 EndRaid 가 모든 PC ??발사. Modal ?�이?�에 결산 ?�젯 push + Configure.
	UFUNCTION(Client, Reliable)
	void Client_ShowRaidEndTransition(bool bSuccess,
	                                  const TArray<FPDPlayerRaidEntryData>& Entries,
	                                  float RaidDurationSeconds);

	// GA_ReviveAbility 가 ?�버?�서 Reviver ??PC ??발사. HUD CircularProgress ?�작/종료.
	// Target ?�터 ?�치�??�라?�니???�젯?�라 RPC ???�봉.
	UFUNCTION(Client, Reliable)
	void Client_NotifyReviveStarted(AActor* Target, float Duration);

	UFUNCTION(Client, Reliable)
	void Client_NotifyReviveEnded(bool bCompleted);

	// 서버 StartRaid 가 모든 PC 에 발사. Modal 레이어에 라이드 진입 연출 위젯 push + Configure.
	UFUNCTION(Client, Reliable)
	void Client_ShowRaidStartTransition(const FText& ZoneName);

	UFUNCTION(BlueprintCallable, Category = "PD|Stash")
	void OpenStashInterface(UPDStashComponent* StashSource);

	UFUNCTION(Client, Reliable)
	void ClientOpenStashInterface(UPDStashComponent* StashSource);

	UFUNCTION(Client, Reliable)
	void ClientCloseStashInterface(UPDStashComponent* StashSource);

	UFUNCTION(BlueprintCallable, Category = "PD|Stash")
	void CloseStashInterface();

	UFUNCTION(Server, Reliable)
	void ServerNotifyStashInterfaceClosed(UPDStashComponent* StashSource);

	UFUNCTION(BlueprintPure, Category = "PD|Stash")
	bool IsStashInterfaceOpen() const;


	UFUNCTION(BlueprintPure, Category = "PD|Stash")
	UPDStashComponent* GetActiveStashComponent() const;

	// === Source legacy: ?�터?�이???�힘 ?�리게이??===
	FPDStashInterfaceClosedSignature OnStashInterfaceClosed;
	FPDMarketInterfaceClosedSignature OnMarketInterfaceClosed;
	FPDEquipmentModificationInterfaceClosedSignature OnEquipmentModificationInterfaceClosed;

	// ?�?�?� LootBox ?�터?�이???�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�
	// Stash ?� ?�전??분리???�스????UPDLootComponent 백엔??
	FPDLootInterfaceClosedSignature OnLootInterfaceClosed;

	UFUNCTION(BlueprintCallable, Category = "PD|Loot")
	void OpenLootInterface(UPDLootComponent* LootSource);

	UFUNCTION(BlueprintCallable, Category = "PD|Loot")
	void CloseLootInterface();

	// 위젯 생성/제거는 소유 클라(로컬 PC)에서만 가능 — 서버에서 원격 클라 대상이면 Client RPC 로 위임.
	UFUNCTION(Client, Reliable)
	void ClientOpenLootInterface(UPDLootComponent* LootSource);

	UFUNCTION(Client, Reliable)
	void ClientCloseLootInterface(UPDLootComponent* LootSource);

	// Loot 컨테이너 전송(서버 권위) — LootComponent 메서드가 클라에서 호출되면 여기로 라우팅.
	UFUNCTION(Server, Reliable)
	void ServerTakeLootSlotToInventory(UPDLootComponent* LootComponent, int32 LootSlotIndex, int32 Quantity);

	UFUNCTION(Server, Reliable)
	void ServerTakeLootSlotQuantityToInventorySlot(UPDLootComponent* LootComponent, int32 LootSlotIndex, int32 TargetInventorySlotIndex, int32 Quantity);

	UFUNCTION(Server, Reliable)
	void ServerStoreInventorySlotQuantityToLoot(UPDLootComponent* LootComponent, int32 SourceSlotIndex, int32 TargetLootSlotIndex, int32 Quantity);

	UFUNCTION(Server, Reliable)
	void ServerMoveLootSlotQuantity(UPDLootComponent* LootComponent, int32 SourceSlotIndex, int32 TargetSlotIndex, int32 Quantity);

	UFUNCTION(BlueprintPure, Category = "PD|Loot")
	bool IsLootInterfaceOpen() const;

	UFUNCTION(BlueprintPure, Category = "PD|Loot")
	FORCEINLINE UPDLootComponent* GetActiveLootComponent() const { return ActiveLootComponent.Get(); }
	// ?�?�?� LootBox ?�터?�이?????�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�

	UFUNCTION(BlueprintCallable, Category = "PD|Market")
	void OpenMarketInterface(UPDMarketComponent* MarketComponent);

	UFUNCTION(Client, Reliable)
	void ClientOpenMarketInterface(UPDMarketComponent* MarketComponent);

	UFUNCTION(BlueprintCallable, Category = "PD|Market")
	void CloseMarketInterface();

	UFUNCTION(BlueprintPure, Category = "PD|Market")
	bool IsMarketInterfaceOpen() const;

	UFUNCTION(BlueprintPure, Category = "PD|Market")
	UPDMarketComponent* GetActiveMarketComponent() const;

	UFUNCTION(BlueprintCallable, Category = "PD|Market")
	bool SellInventorySlotToActiveMarket(int32 SlotIndex, int32 Quantity = 1);

	UFUNCTION(Server, Reliable)
	void ServerBuyMarketEntry(UPDMarketComponent* MarketComponent, int32 EntryIndex, int32 Quantity);

	UFUNCTION(Server, Reliable)
	void ServerSellInventorySlotToMarket(UPDMarketComponent* MarketComponent, int32 SlotIndex, int32 Quantity);

	UFUNCTION(Server, Reliable)
	void ServerUpgradeStash(UPDStashComponent* StashComponent);

	UFUNCTION(Server, Reliable)
	void ServerStoreInventorySlotQuantityToStash(UPDStashComponent* StashComponent, int32 SourceSlotIndex, int32 TargetStashSlotIndex, int32 Quantity);

	UFUNCTION(Server, Reliable)
	void ServerTakeStashSlotQuantity(UPDStashComponent* StashComponent, int32 StashSlotIndex, int32 Quantity);

	UFUNCTION(Server, Reliable)
	void ServerTakeStashSlotQuantityToInventorySlot(UPDStashComponent* StashComponent, int32 StashSlotIndex, int32 TargetInventorySlotIndex, int32 Quantity);

	UFUNCTION(Server, Reliable)
	void ServerMoveStashSlotQuantity(UPDStashComponent* StashComponent, int32 SourceSlotIndex, int32 TargetSlotIndex, int32 Quantity);

	UFUNCTION(Server, Reliable)
	void ServerMoveInventorySlotQuantity(int32 SourceSlotIndex, int32 TargetSlotIndex, int32 Quantity);

	UFUNCTION(Server, Reliable)
	void ServerStoreInventorySlotQuantityToQuickSlot(int32 SourceSlotIndex, int32 TargetQuickSlotIndex, int32 Quantity);

	UFUNCTION(Server, Reliable)
	void ServerMoveQuickSlotQuantity(int32 SourceSlotIndex, int32 TargetSlotIndex, int32 Quantity);

	UFUNCTION(Server, Reliable)
	void ServerEquipInventoryWeaponSlot(int32 InventorySlotIndex);

	UFUNCTION(BlueprintCallable, Category = "PD|Equipment Modification")
	void OpenEquipmentModificationInterface();

	UFUNCTION(BlueprintCallable, Category = "PD|Equipment Modification")
	void CloseEquipmentModificationInterface();

	UFUNCTION(BlueprintPure, Category = "PD|Equipment Modification")
	bool IsEquipmentModificationInterfaceOpen() const;

	UFUNCTION(BlueprintCallable, Category = "PD|Notification")
	void ShowNotification(const FText& Message, float Duration = 2.f);

	UFUNCTION(BlueprintPure, Category = "PD|UI")
	UPDPlayerUIManagerComponent* GetUIManagerComponent() const { return UIManagerComponent; }

	UFUNCTION(BlueprintPure, Category = "PD|PlayerData")
	APDPlayerState* GetPDPlayerState() const;

	UFUNCTION(BlueprintPure, Category = "PD|PlayerData")
	UPDInventoryComponent* GetPlayerInventoryComponent() const;

	UFUNCTION(BlueprintPure, Category = "PD|PlayerData")
	UPDEquipmentComponent* GetPlayerEquipmentComponent() const;

	UFUNCTION(BlueprintPure, Category = "PD|PlayerData")
	UPDQuickSlotComponent* GetPlayerQuickSlotComponent() const;

	UFUNCTION(BlueprintPure, Category = "PD|PlayerData")
	UPDQuestComponent* GetPlayerQuestComponent() const;

	UFUNCTION(BlueprintCallable, Category = "PD|Quest")
	void OpenQuestInterface();

	UFUNCTION(BlueprintCallable, Category = "PD|Quest")
	void CloseQuestInterface();

	UFUNCTION(BlueprintPure, Category = "PD|Quest")
	bool IsQuestInterfaceOpen() const;

	// 주어�?GameplayTag??매핑??InputAction????매핑??IMC?�서 조회.
	// UIOnly 모드?�서 ?�젯??PC InputComponent ?�??직접 ???�력??처리?????�용.
	TArray<FKey> GetMappedKeysForInputTag(const FGameplayTag& InputTag) const;

	virtual void PlayerTick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ?�?�?� Spectator (Step 2-B: ?�망 ??ViewTarget ?�환 관?? ?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�
	// ?�버?�서 GameMode::OnPlayerDied 가 ?�출. �??�존???�?�으�?관???�작.
	void StartSpectatingDeath(APlayerController* InitialTarget);
	// ?�버?�서 ?�출. ?�신??관???�?�이 DeadPC �??�음 ?�존?�로 ?�동.
	void CycleSpectateIfTargetIs(APlayerController* AffectedPC);

	UFUNCTION(BlueprintPure, Category = "PD|Spectator")
	bool IsSpectating() const { return bIsSpectating; }

	UFUNCTION(BlueprintPure, Category = "PD|Spectator")
	APlayerController* GetSpectateTargetController() const { return SpectateTargetPC; }

	UFUNCTION(BlueprintPure, Category = "PD|Spectator")
	FString GetSpectateTargetName() const;

	UPROPERTY(BlueprintAssignable, Category = "PD|Spectator")
	FPDSpectateChangedDelegate OnSpectateChanged;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|UI")
	TSubclassOf<UPDHUDWidget> HUDClass;


	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|UI")
	TSubclassOf<UPDRootLayout> RootLayoutClass;

	virtual void CreateAndAddHUDWidget();

	UPROPERTY(EditAnywhere, Category = "PD|Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditAnywhere, Category = "PD|Input")
	TObjectPtr<UPDInputConfig> InputConfig;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ping")
	TObjectPtr<UPDPingInputComponent> PingInputComp;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="WorldMap")
	TSubclassOf<UPDWorldMapWidget> WorldMapClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|UI")
	TSubclassOf<UPDInventoryWidget> InventoryWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|UI")
	TSubclassOf<UPDStashWidget> StashWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|UI")
	TSubclassOf<UPDLootWidget> LootWidgetClass; // LootBox??
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|UI")
	TSubclassOf<UPDMarketWidget> MarketWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|UI")
	TSubclassOf<UUserWidget> EquipmentModificationWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|UI")
	TSubclassOf<UPDNotificationWidget> NotificationWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|UI", meta = (ClampMin = "0.1"))
	float NotificationDuration = 2.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|UI")
	int32 NotificationZOrder = 50;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|UI")
	TSubclassOf<UPDQuestWindowWidget> QuestWindowWidgetClass;

	// 결산 ?�젯 ?�래?? BP_PDPlayerController ?�서 WBP_PDRaidEndTransition ?�당.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|UI|Transition")
	TSubclassOf<UPDRaidEndTransitionWidget> RaidEndTransitionClass;

	// 라이드 진입 연출 위젯 클래스. BP_PDPlayerController 에서 WBP_PDRaidStartTransition 할당.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|UI|Transition")
	TSubclassOf<UPDRaidStartTransitionWidget> RaidStartTransitionClass;

	// 진입 연출 push 재시도 — PostLogin 시점 RPC 가 RootLayout 등록 전 도착할 수 있어 UI 준비까지 대기.
	void TryShowRaidStartTransition();
	FText PendingRaidStartZoneName;
	FTimerHandle RaidStartTransitionRetryHandle;
	int32 RaidStartTransitionRetryCount = 0;
	bool bHasPendingRaidStartTransition = false;

	/** Quip(캐릭터 멘트) 데이터. UPDQuipSubsystem에 주입되어 태그→Line 라우팅에 사용. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|UI")
	TSoftObjectPtr<UPDQuipDataAsset> QuipDataAsset;

	virtual void SetupInputComponent() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnRep_PlayerState() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void PreClientTravel(const FString& PendingURL, ETravelType TravelType, bool bIsSeamlessTravel) override;

public:

	void AddRecoilOffset(float YawDelta);

	UFUNCTION(Client, Unreliable)
	void ClientAddRecoilOffset(float YawDelta);

	FORCEINLINE float GetRecoilYawOffset() const { return RecoilYawOffset; }
	bool GetRecoiledHitResult(ECollisionChannel TraceChannel, bool bTraceComplex, FHitResult& OutHit) const;
	bool GetRecoiledHitResultForObjects(const TArray<TEnumAsByte<EObjectTypeQuery>>& ObjectTypes, bool bTraceComplex, FHitResult& OutHit) const;
	bool GetCachedAimWorldLocation(FVector& OutLocation) const;

	UFUNCTION(Server, Reliable)
	void ServerTryActivateAbilityByTag(FGameplayTag InputTag);

	UFUNCTION(Server, Reliable)
	void ServerCancelAbilityByTag(FGameplayTag InputTag);

	UFUNCTION(Server, Reliable)
	void ServerHandleGameplayEvent(FGameplayTag EventTag);

	UFUNCTION(Server, Reliable)
	void ServerToggleFireMode();

	UFUNCTION(Server, Unreliable)
	void ServerSetAimWorldLocation(FVector AimLocation, FVector FireAimLocation);

	void OnFireReleased();

private:

	float RecoilYawOffset = 0.f;
	FVector2D RecoilCursorOffset = FVector2D::ZeroVector;


	UPROPERTY(EditDefaultsOnly, Category="PD|Recoil", meta=(ClampMin="0.0"))
	float RecoilRecoverySpeed = 10.f;

	UPROPERTY(EditDefaultsOnly, Category="PD|Recoil", meta=(ClampMin="0.0"))
	float RecoilCursorPixelsPerDegree = 18.f;

	UPROPERTY(EditDefaultsOnly, Category="PD|Recoil", meta=(ClampMin="0.0"))
	float RecoilCursorHorizontalRatio = 0.35f;

	UPROPERTY(EditDefaultsOnly, Category="PD|Recoil", meta=(ClampMin="0.0"))
	float MaxRecoilCursorOffset = 120.f;

	FVector CachedAimWorldLocation = FVector::ZeroVector;
	bool bHasCachedAimWorldLocation = false;

	void TickRecoilRecovery(float DeltaTime);
	bool GetRecoiledMousePosition(FVector2D& OutMousePosition) const;

	void UseQuickSlot(int32 SlotIndex);

	void OnMove(const struct FInputActionValue& Value);
	void OnJump();
	void OnAbilityInputPressed(FGameplayTag InputTag);
	void OnAbilityInputReleased(FGameplayTag InputTag);
	void CancelAbilityByInputTag(FGameplayTag InputTag);
	void UpdateAimRotation();

	// Hub ?�합 ?�면 ?��?. Input_Inventory ?�그??매핑?????�력 ???�출.
	void ToggleHub();

	void TryInteract();
	void StopInteract();

	void OnToggleWorldMap();

	bool IsGameplayInputBlockedByModalUI() const;
	bool ShouldAllowMovementWhileUIOpen() const;
	void SetGameplayInputBlockedByModalUI(bool bBlocked, UUserWidget* WidgetToFocus = nullptr);


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="PD|UI", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UPDPlayerUIManagerComponent> UIManagerComponent;


	bool IsGameplayInputBlockedByModalUI() const;
	void SetGameplayInputBlockedByModalUI(bool bBlocked, UUserWidget* WidgetToFocus = nullptr);

	UPROPERTY(Transient)
	TObjectPtr<UPDInventoryComponent> BoundInventoryNotificationComponent;

	// 실제 위젯 생성/제거 — 항상 로컬 PC 에서 실행(라우팅은 Open/CloseLootInterface 가 담당).
	void OpenLootInterfaceLocal(UPDLootComponent* LootSource);
	void CloseLootInterfaceLocal();

	UPROPERTY(Transient)
	TObjectPtr<UPDLootWidget> LootWidgetInstance;

	// OpenLootInterface ??캐시. 박스가 ?�괴?�어??TWeakObjectPtr가 ?�동 무효??
	TWeakObjectPtr<UPDLootComponent> ActiveLootComponent;



<<<<<<< HEAD
	void ApplyEffectiveUIState(EWidgetInputMode Mode);

	void OnFirePressed();
	void OnReload();
=======
	bool bIsGameplayInputBlockedByModalUI = false;
	bool bMouseCursorVisibleBeforeModalUI = false;

>>>>>>> origin/main
	void OnInteract();
	void OnSwitchSlot1();
	void OnSwitchSlot2();
	void OnSwitchSlot3();
	void OnUseQuickSlot4();
	void OnQuickslot1();
	void OnQuickslot2();
	void OnQuickslot3();
	void OnQuickslot4();
	void OnQuickslot5();
	void OnQuickslot6();
	void OnCancelConsumableUse();
	void SelectQuickslot(int32 Index);
	void OnToggleFireMode();
	void OnDropWeapon();
	void OnAimPressed();
	void UpdateCrosshair();

<<<<<<< HEAD
	UFUNCTION()
	void OnWeaponChanged(APDWeaponBase* NewWeapon, EWeaponSlot Slot);
=======
	bool bMouseClickEventsEnabledBeforeModalUI = false;
	bool bMouseOverEventsEnabledBeforeModalUI = false;
>>>>>>> origin/main

	void BindInventoryNotifications();
	void UnbindInventoryNotifications();

	UFUNCTION()
	void HandleInventoryMessage(const FText& Message);

	UFUNCTION()
	void HandleInventoryWeightLimitExceeded(float CurrentWeight, float MaxWeight);

	// ?�?�?� Spectator internals ?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�
	UFUNCTION(Server, Reliable)
	void Server_SpectateNext();

	UFUNCTION(Server, Reliable)
	void Server_SpectatePrev();

	void OnSpectateNextInput();
	void OnSpectatePrevInput();

	// ?�버 권한 가?? Direction = +1(next) / -1(prev). ?�보가 ?�으�?관??종료.
	void CycleSpectateTargetServer(int32 Direction);

	UFUNCTION()
	void OnRep_SpectateState();

	UPROPERTY(ReplicatedUsing = OnRep_SpectateState, Transient)
	bool bIsSpectating = false;

	UPROPERTY(ReplicatedUsing = OnRep_SpectateState, Transient)
	TObjectPtr<APlayerController> SpectateTargetPC;
};
