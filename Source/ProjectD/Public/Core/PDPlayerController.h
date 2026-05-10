#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PDPlayerController.generated.h"

struct FGameplayTag;
class UNiagaraSystem;
class UInputMappingContext;
class UInputAction;
class UPathFollowingComponent;
class UPDInventoryWidget;
class UPDStashWidget;
class UPDMarketWidget;
class UPDMarketComponent;
class APDPlayerCharacter;
class UPDHUDWidget;
class UPDActivatableBase;

DECLARE_LOG_CATEGORY_EXTERN(LogPDCharacter, Log, All);

class UInputMappingContext;
class UPDInputConfig;

class APDPlayerCharacter;
class APDWeaponBase;       
class APDRifle;           

UCLASS(abstract)
class PROJECTD_API APDPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	APDPlayerController();

	UFUNCTION(BlueprintCallable, Category = "PD|Raid")
	void RequestExtraction();

	UFUNCTION(BlueprintCallable, Category = "PD|Stash")
	void OpenStashInterface();

	UFUNCTION(BlueprintCallable, Category = "PD|Stash")
	void CloseStashInterface();

	UFUNCTION(BlueprintPure, Category = "PD|Stash")
	bool IsStashInterfaceOpen() const;

	UFUNCTION(BlueprintCallable, Category = "PD|Market")
	void OpenMarketInterface(UPDMarketComponent* MarketComponent);

	UFUNCTION(BlueprintCallable, Category = "PD|Market")
	void CloseMarketInterface();

	UFUNCTION(BlueprintPure, Category = "PD|Market")
	bool IsMarketInterfaceOpen() const;

	UFUNCTION(BlueprintCallable, Category = "PD|Market")
	bool SellInventorySlotToActiveMarket(int32 SlotIndex, int32 Quantity = 1);

	virtual void PlayerTick(float DeltaTime) override; 

protected:
	/** HUD 위젯 클래스. BeginPlay에서 자동 생성되어 viewport ZOrder=0에 깔린다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|UI")
	TSubclassOf<UPDHUDWidget> HUDClass;

	/** HUD 위젯 생성 + viewport 추가. BeginPlay에서 호출. 자식 PC가 override 가능. */
	virtual void CreateAndAddHUDWidget();

	/** 현재 열려 있는 화면을 닫고 싶을 때 BP에서 호출. */
	UFUNCTION(BlueprintCallable, Category = "PD|UI")
	void RequestCloseCurrentScreen();

	UPROPERTY(EditAnywhere, Category = "PD|Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditAnywhere, Category = "PD|Input")
	TObjectPtr<UPDInputConfig> InputConfig;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|UI")
	TSubclassOf<UPDInventoryWidget> InventoryWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|UI")
	TSubclassOf<UPDStashWidget> StashWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|UI")
	TSubclassOf<UPDMarketWidget> MarketWidgetClass;

	virtual void SetupInputComponent() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
private:
	void OnMove(const struct FInputActionValue& Value);
	void OnJump();
	void OnAbilityInputPressed(FGameplayTag InputTag);
	void OnAbilityInputReleased(FGameplayTag InputTag);
	void UpdateAimRotation();

	void ToggleInventory();
	void TryInteract();

	UPROPERTY(Transient)
	TObjectPtr<UPDInventoryWidget> InventoryWidgetInstance;

	UPROPERTY(Transient)
	TObjectPtr<UPDStashWidget> StashWidgetInstance;

	UPROPERTY(Transient)
	TObjectPtr<UPDMarketWidget> MarketWidgetInstance;

	UPROPERTY(Transient)
	TObjectPtr<UPDMarketComponent> ActiveMarketComponent;

	UPROPERTY(Transient)
	TObjectPtr<UPDHUDWidget> HUDInstance;

	void OnInteract();
	void OnSwitchSlot1();
	void OnSwitchSlot2();
	void OnSwitchSlot3();
	void OnZoom();
	void OnToggleFireMode();
	void OnDropWeapon();

	bool bShowMouseCursor=true;

};
