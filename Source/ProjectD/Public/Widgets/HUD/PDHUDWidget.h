// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Widgets/PDActivatableBase.h"
#include "AttributeSet.h"
#include "Type/Types.h"
#include "PDHUDWidget.generated.h"

class UPDAttributeBarWidget;
class UPDBodyPartHealthGroupWidget;
class UPDNewQuickSlotBarWidget;
class UPDDebuffIconBarWidget;
class UPDCrosshairWidget;
class UPDGasMaskWidget;
class UPDActionPromptListWidget;
class UPDInteractPromptWidget;
class UPDSkillSlotBarWidget;
class UPDCircularProgressWidget;
class UPDQuipToastWidget;
class UPDMainWeaponAmmoWidget;
class UPDQuickSlotComponent;
class UPDInteractionComponent;
class UAbilitySystemComponent;
class UTextBlock;
class APDPlayerController;
struct FOnAttributeChangeData;
enum class EWeaponType : uint8;

/**
 * 플레이어 HUD 위젯
 * Activate 시점에 Pawn의 ASC를 잡아 attribute/tag 변경 델리게이트를 구독하고, Deactivate 시점에 구독을 해제.
 * PC가 possession 변경 시 RebindToASC로 재바인딩.
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class PROJECTD_API UPDHUDWidget : public UPDActivatableBase
{
	GENERATED_BODY()

public:
	UPDHUDWidget(const FObjectInitializer& ObjectInitializer);
	
	void RebindToASC(UAbilitySystemComponent* NewASC);

protected:
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UPDBodyPartHealthGroupWidget> Bar_BodyParts;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UPDAttributeBarWidget> Bar_Stamina;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UPDAttributeBarWidget> Bar_Hunger;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UPDAttributeBarWidget> Bar_Thirst;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UPDGasMaskWidget> Bar_GasMask;


	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UPDNewQuickSlotBarWidget> Bar_NewQuickSlots;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UPDDebuffIconBarWidget> Bar_Debuffs;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UPDActionPromptListWidget> Bar_ActionPrompts;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UPDInteractPromptWidget> WBP_InteractPrompt;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UPDSkillSlotBarWidget> Bar_SkillSlots;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UPDCrosshairWidget> WBP_Crosshair;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UPDCircularProgressWidget> WBP_UseProgress;

	// Revive 진행도 — 대상 위치 따라 스크린 좌표로 따라다님. WBP_UseProgress 와 별도 인스턴스.
	// WBP 에서 작은 크기로 만들어 두면 시각적으로 차별화됨.
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UPDCircularProgressWidget> WBP_ReviveProgress;

	// Revive 진행도 위젯이 대상 액터의 어느 위치에 표시될지. 액터 위치 + 이 오프셋 → 스크린 투영.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|HUD|Revive")
	FVector ReviveProgressWorldOffset = FVector(0.f, 0.f, 80.f);

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UPDQuipToastWidget> WBP_QuipToast;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UPDMainWeaponAmmoWidget> WBP_MainWeaponAmmo;

	// Step 2-B: 사망 후 관전 모드일 때 "Spectating: <PlayerName>" 표시. BP 에서 선택적으로 추가.
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_SpectateTarget;

public:
	UFUNCTION(BlueprintCallable, Category = "PD|HUD")
	void RefreshNewQuickSlots();

	// PC가 매 틱 호출. 내부 크로스헤어로 위임.
	void UpdateCrosshair(FVector2D MousePos, float Spread);

	// 무기 교체 시 호출.
	void SetCrosshairType(EWeaponType NewType);

	// 입력 모드 전환 시 표시/숨김.
	void SetCrosshairVisible(bool bVisible);

	FORCEINLINE UPDCrosshairWidget* GetCrosshair() const { return WBP_Crosshair; }

	// Revive 진행도 — Reviver 측 PC 가 Client RPC 로 호출. Target 위치에 따라다님.
	UFUNCTION(BlueprintCallable, Category = "PD|HUD|Revive")
	void StartReviveProgress(AActor* Target, float Duration);

	UFUNCTION(BlueprintCallable, Category = "PD|HUD|Revive")
	void StopReviveProgress();

	UFUNCTION(BlueprintCallable, Category = "PD|HUD|Revive")
	void CompleteReviveProgress();

private:
	struct FBoundAttributeHandle
	{
		FGameplayAttribute Attribute;
		FDelegateHandle Handle;
	};

	struct FBoundTagHandle
	{
		FGameplayTag Tag;
		FDelegateHandle Handle;
	};

	void BindAllAttributes();
	void BindAttributeToBar(UPDAttributeBarWidget* Bar,
		const FGameplayAttribute& CurrentAttr,
		const FGameplayAttribute& MaxAttr);
	void RefreshBar(UPDAttributeBarWidget* Bar,
		const FGameplayAttribute& CurrentAttr,
		const FGameplayAttribute& MaxAttr) const;
	void UnbindAllAttributes();

	void BindAllDebuffTags();
	void BindTagEvent(const FGameplayTag& Tag,
		TFunction<void(const FGameplayTag&, int32)> OnChanged);
	void UnbindAllTags();
	
	void HandleBleeding(const FGameplayTag& Tag, int32 NewCount);
	void HandleLegDamaged(const FGameplayTag& Tag, int32 NewCount);
	void HandleArmDamaged(const FGameplayTag& Tag, int32 NewCount);
	void HandleStarving(const FGameplayTag& Tag, int32 NewCount);
	void HandleDehydrated(const FGameplayTag& Tag, int32 NewCount);

	// Revive 진행도 위젯이 따라다닐 대상 + 타이머. 매 프레임 ScreenPos 재계산.
	void UpdateReviveProgressPosition();
	TWeakObjectPtr<AActor> CachedReviveTarget;
	FTimerHandle ReviveProgressUpdateTimer;
	// Revive 중에는 InteractPrompt 갱신 억제 — 같은 target 에 두 위젯 겹치는 거 방지.
	bool bSuppressInteractPrompt = false;

	UFUNCTION()
	void HandleConsumableUseStarted(int32 SlotIndex, FPDItemData ItemData, float Duration);

	UFUNCTION()
	void HandleConsumableUseCanceled(int32 SlotIndex, FPDItemData ItemData);

	UFUNCTION()
	void HandleConsumableUseCompleted(int32 SlotIndex, FPDItemData ItemData);

	UPDQuickSlotComponent* FindOwningQuickSlotComponent() const;
	void RefreshUseProgressBinding(UPDQuickSlotComponent* NewComponent);

	UPDInteractionComponent* FindOwningInteractionComponent() const;
	void RefreshInteractPromptBinding(UPDInteractionComponent* NewComponent);

	UFUNCTION()
	void HandleInteractTargetChanged(AActor* NewTarget);

	// 매 프레임 호출되는 위치 갱신(타이머 기반).
	void UpdateInteractPromptPosition();

	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;
	TWeakObjectPtr<UPDQuickSlotComponent> CachedQuickSlot;
	TWeakObjectPtr<UPDInteractionComponent> CachedInteraction;
	TWeakObjectPtr<AActor> CachedInteractTarget;
	FTimerHandle InteractPromptUpdateTimer;
	TArray<FBoundAttributeHandle> BoundAttributeHandles;
	TArray<FBoundTagHandle> BoundTagHandles;

	// Step 2-B: 관전 모드 HUD 표시.
	void BindSpectatorDelegate(APDPlayerController* PC);
	void UnbindSpectatorDelegate();
	UFUNCTION()
	void HandleSpectateChanged();
	void RefreshSpectateDisplay();

	TWeakObjectPtr<APDPlayerController> CachedSpectatorPC;
};
