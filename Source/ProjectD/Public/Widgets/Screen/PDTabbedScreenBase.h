#pragma once

#include "CoreMinimal.h"
#include "Widgets/Screen/PDScreenTemplateBase.h"
#include "GameplayTagContainer.h"
#include "Type/Types.h"
#include "PDTabbedScreenBase.generated.h"

class UHorizontalBox;
class UWidgetSwitcher;
class UPDTabbedScreenDataAsset;
class UPDTabButtonWidget;

/**
 * 상단 탭 + 중앙 WidgetSwitcher 구성의 탭 화면.
 * 화면이 무엇을 보여줄지는 TabSet DataAsset이 결정한다(코드는 screen-agnostic).
 *
 * 새 화면 추가 = 이 클래스 상속한 WBP + 새 DataAsset. C++ 수정 없음.
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class PROJECTD_API UPDTabbedScreenBase : public UPDScreenTemplateBase
{
	GENERATED_BODY()

public:
	/** 화면을 push하고 지정 탭으로 초기화하는 헬퍼. */
	UFUNCTION(BlueprintCallable, Category = "PD|Tabbed", meta = (WorldContext = "WorldContextObject"))
	static UPDTabbedScreenBase* OpenAtTab(
		UObject* WorldContextObject,
		TSubclassOf<UPDTabbedScreenBase> ScreenClass,
		FGameplayTag TabId,
		EUILayer Layer = EUILayer::GameMenu);

	/** Activate 전에 호출되면 초기 탭으로 적용. Activate 후 호출 시 즉시 전환. */
	UFUNCTION(BlueprintCallable, Category = "PD|Tabbed")
	void SetInitialTab(FGameplayTag TabId);

	UFUNCTION(BlueprintCallable, Category = "PD|Tabbed")
	void SwitchToTab(FGameplayTag TabId);

	/** Direction +1/-1. 비활성 탭은 건너뜀. */
	UFUNCTION(BlueprintCallable, Category = "PD|Tabbed")
	void CycleTab(int32 Direction);

	UFUNCTION(BlueprintPure, Category = "PD|Tabbed")
	FGameplayTag GetActiveTabId() const { return ActiveTabId; }

protected:
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Tabbed")
	TObjectPtr<UPDTabbedScreenDataAsset> TabSet;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Tabbed")
	TSubclassOf<UPDTabButtonWidget> TabButtonClass;

	/** 탭 버튼들이 들어가는 컨테이너. WBP에서 BindWidget. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UHorizontalBox> HBox_TabBar;

	/** 컨텐츠 패널들이 들어가는 스위처. WBP에서 BindWidget. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UWidgetSwitcher> Switcher_Content;

private:
	UFUNCTION()
	void HandleTabButtonClicked(FGameplayTag ClickedTabId);

	void RebuildTabsAndContent();
	void ClearTabsAndContent();
	int32 FindTabIndexInDataAsset(FGameplayTag TabId) const;
	void ApplyActiveTabVisualState();
	FGameplayTag ResolveInitialTabId() const;

	UPROPERTY(Transient)
	TMap<FGameplayTag, TObjectPtr<UUserWidget>> SpawnedContents;

	UPROPERTY(Transient)
	TMap<FGameplayTag, TObjectPtr<UPDTabButtonWidget>> SpawnedButtons;

	FGameplayTag ActiveTabId;
	FGameplayTag PendingInitialTab;
	FGameplayTag LastTabId;
};