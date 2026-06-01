// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "Blueprint/UserWidget.h"
#include "PDActivatableBase.generated.h"

// 각 위젯이 활성화될 때 게임 입력을 어떻게 처리할지 정의하는 열거형
UENUM(BlueprintType)
enum class EWidgetInputMode : uint8
{
	Game        UMETA(DisplayName = "Game Only"),       // 게임 입력 그대로
	GameAndMenu UMETA(DisplayName = "Game And Menu"),   // HUD 위에 떠도 게임 계속
	Menu        UMETA(DisplayName = "Menu Only"),       // 인벤토리/맵 — 게임 입력 차단, 마우스 노출
	Passive     UMETA(DisplayName = "Passive"),         // 입력 모드/커서 상태를 건드리지 않음
	Cinematic   UMETA(DisplayName = "Cinematic")        // 연출 — 게임 입력 차단 + 커서 숨김 (트랜지션/컷신)
};

UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class PROJECTD_API UPDActivatableBase : public UUserWidget
{
	GENERATED_BODY()

public:
	/** UGameUIManagerSubsystem::PushWidget에 의해 호출. InputMode/Cursor 적용 + OnActivated 호출 */
	UFUNCTION(BlueprintCallable, Category = "PD|UI")
	void Activate();

	/** UGameUIManagerSubsystem::PopWidget에 의해 호출. OnDeactivated 호출 */
	UFUNCTION(BlueprintCallable, Category = "PD|UI")
	void Deactivate();

	/** push 직후 포커스 후보. 기본 nullptr. 서브클래스/BP가 오버라이드. */
	UFUNCTION(BlueprintNativeEvent, Category = "PD|UI")
	UWidget* GetDesiredFocusTarget() const;

	bool IsActivated() const { return bActivated; }

	/** Subsystem이 효과적 입력 모드 계산을 위해 top 위젯의 선언값을 조회. */
	EWidgetInputMode GetInputMode() const { return InputMode; }

protected:
	/** C++에서 위젯 활성화 시, 넣고 싶은 로직 작성*/
	virtual void NativeOnActivated();

	/** C++에서 위젯 비활성화 시, 넣고 싶은 로직 작성*/
	virtual void NativeOnDeactivated();

	virtual void NativeOnFocusLost(const FFocusEvent& InFocusEvent) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	/**
	 * UIBack(ESC 등) 입력 시 호출. true 반환 시 이벤트 소비.
	 * 기본 구현: this가 어느 레이어의 top이면 소비(true). bAllowEscapeDismiss면 그 레이어 pop.
	 * → 연출 위젯(RaidTransition 등)은 bAllowEscapeDismiss=false로 두면 ESC를 막되 닫히지 않음.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "PD|UI")
	bool HandleEscape();
	virtual bool HandleEscape_Implementation();

	/** Input_UIBack 매핑 키인지 동적 판정(IMC 주도). 매핑 없으면 Escape 폴백. */
	bool IsUIBackKey(const FKey& InKey) const;

	/** BP에서 위젯 활성화 시, 넣고 싶은 로직 작성 */
	UFUNCTION(BlueprintImplementableEvent, Category = "PD|UI", DisplayName = "On Activated")
	void OnActivated();

	/** BP에서 위젯 비활성화 시, 넣고 싶은 로직 작성 */
	UFUNCTION(BlueprintImplementableEvent, Category = "PD|UI", DisplayName = "On Deactivated")
	void OnDeactivated();

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	EWidgetInputMode InputMode{EWidgetInputMode::Menu};

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	bool bShowMouseCursor{true};

	/** 포커스를 잃었을 때 위젯을 닫을지 여부 */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	bool bLightDismissable{false};

	/** UIBack(ESC)로 이 위젯을 pop할 수 있는지. false면 ESC를 소비하되 닫지 않음(연출/강제 모달 보호). */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	bool bAllowEscapeDismiss{true};

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	bool bAllowHitTestPassThroughOnRoot{false};

public:
	virtual bool ShouldPassThroughRootHitTest() const { return bAllowHitTestPassThroughOnRoot; }

private:
	void ApplyInputMode();

	bool bActivated{false};
};
