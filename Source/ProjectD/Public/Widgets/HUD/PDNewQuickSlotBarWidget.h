#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InputCoreTypes.h"
#include "Type/Types.h"
#include "TimerManager.h"
#include "PDNewQuickSlotBarWidget.generated.h"

class UPDQuickSlotComponent;
class UPDNewQuickSlotItemWidget;
class UInputAction;
class UInputMappingContext;
class UPDKeyIconDataAsset;

UCLASS(BlueprintType, Blueprintable)
class PROJECTD_API UPDNewQuickSlotBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="PD|QuickSlot")
	void BindQuickSlotComponent(UPDQuickSlotComponent* InQuickSlotComponent);

	UFUNCTION(BlueprintCallable, Category="PD|QuickSlot")
	void RefreshSlots();

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	TObjectPtr<UPDNewQuickSlotItemWidget> Slot_0;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	TObjectPtr<UPDNewQuickSlotItemWidget> Slot_1;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	TObjectPtr<UPDNewQuickSlotItemWidget> Slot_2;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	TObjectPtr<UPDNewQuickSlotItemWidget> Slot_3;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	TObjectPtr<UPDNewQuickSlotItemWidget> Slot_4;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	TObjectPtr<UPDNewQuickSlotItemWidget> Slot_5;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="PD|QuickSlot", meta=(ClampMin="0", ClampMax="6"))
	int32 WeaponSlotCount = 2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="PD|QuickSlot|Input")
	TArray<TObjectPtr<UInputAction>> SlotInputActions;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="PD|QuickSlot|Input")
	TObjectPtr<UInputMappingContext> InputMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="PD|QuickSlot|Input")
	TSoftObjectPtr<UPDKeyIconDataAsset> KeyIconMap;

private:
	UFUNCTION()
	void HandleQuickSlotsChanged();

	UFUNCTION()
	void HandleControlMappingsRebuilt();

	UFUNCTION()
	void HandleSelectionChanged(int32 NewIndex);

	UFUNCTION()
	void HandleConsumableUseStarted(int32 SlotIndex, FPDItemData ItemData, float Duration);

	UFUNCTION()
	void HandleConsumableUseCanceled(int32 SlotIndex, FPDItemData ItemData);

	UFUNCTION()
	void HandleConsumableUseCompleted(int32 SlotIndex, FPDItemData ItemData);

	UFUNCTION()
	void HandleWeaponCooldownStarted(int32 SlotIndex, float Duration, float EndTime);

	UFUNCTION()
	void HandleWeaponCooldownFinished(int32 SlotIndex);

	void CollectSlotWidgets();
	void ApplyKeyBindings();
	void ApplySelection(int32 SelectedIndex);
	void StartWeaponCooldownUITimer();
	void StopWeaponCooldownUITimer();
	void UpdateWeaponCooldownUI();
	void SetWeaponCooldownUIForSlot(int32 SlotIndex, float RemainingTime);
	void ClearWeaponCooldownUI();
	bool IsWeaponCooldownUISlot(int32 SlotIndex) const;
	FKey FindKeyForAction(const UInputAction* Action) const;
	UPDQuickSlotComponent* FindQuickSlotComponent() const;

	UPROPERTY(Transient)
	TObjectPtr<UPDQuickSlotComponent> BoundQuickSlotComponent;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UPDNewQuickSlotItemWidget>> SlotWidgets;

	FTimerHandle WeaponCooldownUITimerHandle;
	int32 ActiveWeaponCooldownSlotIndex = INDEX_NONE;
};
