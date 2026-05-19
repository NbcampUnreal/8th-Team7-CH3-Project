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
class UPDQuickSlotComponent;
class UPDInteractionComponent;
class UAbilitySystemComponent;
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

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UPDQuipToastWidget> WBP_QuipToast;

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
};
