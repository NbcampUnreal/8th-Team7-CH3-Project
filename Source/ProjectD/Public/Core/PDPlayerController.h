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
enum class EWidgetInputMode : uint8;

DECLARE_LOG_CATEGORY_EXTERN(LogPDCharacter, Log, All);
DECLARE_MULTICAST_DELEGATE_OneParam(FPDLootInterfaceClosedSignature, UPDLootComponent*);

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

	UFUNCTION(BlueprintCallable, Category = "PD|Stash")
	void OpenStashInterface(UPDStashComponent* StashSource);

	UFUNCTION(Client, Reliable)
	void ClientOpenStashInterface(UPDStashComponent* StashSource);

	UFUNCTION(BlueprintCallable, Category = "PD|Stash")
	void CloseStashInterface();

	UFUNCTION(BlueprintPure, Category = "PD|Stash")
	bool IsStashInterfaceOpen() const;


	UFUNCTION(BlueprintPure, Category = "PD|Stash")
	UPDStashComponent* GetActiveStashComponent() const;

	// ─── LootBox 인터페이스 ──────────────────────────────────────────────
	// Stash 와 완전히 분리된 시스템 — UPDLootComponent 백엔드.
	FPDLootInterfaceClosedSignature OnLootInterfaceClosed;

	UFUNCTION(BlueprintCallable, Category = "PD|Loot")
	void OpenLootInterface(UPDLootComponent* LootSource);

	UFUNCTION(BlueprintCallable, Category = "PD|Loot")
	void CloseLootInterface();

	UFUNCTION(BlueprintPure, Category = "PD|Loot")
	bool IsLootInterfaceOpen() const;

	UFUNCTION(BlueprintPure, Category = "PD|Loot")
	FORCEINLINE UPDLootComponent* GetActiveLootComponent() const { return ActiveLootComponent.Get(); }
	// ─── LootBox 인터페이스 끝 ────────────────────────────────────────────

	UFUNCTION(BlueprintCallable, Category = "PD|Market")
	void OpenMarketInterface(UPDMarketComponent* MarketComponent);

	UFUNCTION(Client, Reliable)
	void ClientOpenMarketInterface(UPDMarketComponent* MarketComponent);

	UFUNCTION(BlueprintCallable, Category = "PD|Market")
	void CloseMarketInterface();

	UFUNCTION(BlueprintPure, Category = "PD|Market")
	bool IsMarketInterfaceOpen() const;

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

	virtual void PlayerTick(float DeltaTime) override;

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
	TSubclassOf<UPDLootWidget> LootWidgetClass; // LootBox용

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

	virtual void SetupInputComponent() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

public:

	void AddRecoilOffset(float YawDelta);

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

	void ToggleInventory();
	void ToggleQuest();
	void TryInteract();
	void StopInteract();

	void OnToggleWorldMap();

	bool IsGameplayInputBlockedByModalUI() const;
	bool ShouldAllowMovementWhileUIOpen() const;
	void SetGameplayInputBlockedByModalUI(bool bBlocked, UUserWidget* WidgetToFocus = nullptr);


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="PD|UI", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UPDPlayerUIManagerComponent> UIManagerComponent;


	UPROPERTY(Transient)
	TObjectPtr<UPDInventoryComponent> BoundInventoryNotificationComponent;

	UPROPERTY(Transient)
	TObjectPtr<UPDLootWidget> LootWidgetInstance;

	// OpenLootInterface 시 캐시. 박스가 파괴되어도 TWeakObjectPtr가 자동 무효화.
	TWeakObjectPtr<UPDLootComponent> ActiveLootComponent;



	void ApplyEffectiveUIState(EWidgetInputMode Mode);

	void OnFirePressed();
	void OnReload();
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

	UFUNCTION()
	void OnWeaponChanged(APDWeaponBase* NewWeapon, EWeaponSlot Slot);

	void BindInventoryNotifications();
	void UnbindInventoryNotifications();

	UFUNCTION()
	void HandleInventoryMessage(const FText& Message);
};
