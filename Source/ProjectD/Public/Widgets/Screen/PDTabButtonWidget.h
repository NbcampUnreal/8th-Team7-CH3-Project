#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "PDTabButtonWidget.generated.h"

class UButton;
class UTextBlock;
class UImage;
class UTexture2D;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPDTabButtonClicked, FGameplayTag, TabId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPDTabSelectionChanged, FGameplayTag, TabId, bool, bIsSelected);

/**
 * 탭 한 개의 버튼 위젯. TabbedScreenBase가 DA를 순회하며 인스턴스를 생성/구성한다.
 * 선택 강조는 BP_OnSelectionChanged에서 UV 패닝 등으로 처리(밝기 변조 금지).
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class PROJECTD_API UPDTabButtonWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "PD|Tab")
	void InitializeTabButton(FGameplayTag InTabId, const FText& InLabel, UTexture2D* OptionalIcon, bool bInEnabled);

	UFUNCTION(BlueprintCallable, Category = "PD|Tab")
	void SetSelected(bool bInSelected);

	UFUNCTION(BlueprintPure, Category = "PD|Tab")
	FGameplayTag GetTabId() const { return TabId; }

	UFUNCTION(BlueprintPure, Category = "PD|Tab")
	bool IsSelected() const { return bSelected; }

	UPROPERTY(BlueprintAssignable, Category = "PD|Tab")
	FOnPDTabButtonClicked OnTabButtonClicked;

	/** 선택 상태가 바뀔 때 broadcast. 외부 위젯(Hub/사이드패널 등)이 이벤트 그래프에서 구독 가능. */
	UPROPERTY(BlueprintAssignable, Category = "PD|Tab")
	FOnPDTabSelectionChanged OnSelectionChanged;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> BTN_Tab;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TXT_Label;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> IMG_Icon;

	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Tab", DisplayName = "On Selection Changed")
	void BP_OnSelectionChanged(bool bInSelected);

	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Tab", DisplayName = "On Enabled State Changed")
	void BP_OnEnabledStateChanged(bool bInEnabled);

private:
	UFUNCTION()
	void HandleButtonClicked();

	FGameplayTag TabId;
	bool bSelected = false;
	bool bEnabled = true;
};