#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InputCoreTypes.h"
#include "PDCircularProgressWidget.generated.h"

class UImage;
class UTextBlock;
class UHorizontalBox;
class UInputAction;
class UInputMappingContext;
class UMaterialInstanceDynamic;
class UPDKeyIconDataAsset;

/**
 * 채널링 게이지 — 화면 중앙(퀵슬롯 바 위)에 떠있는 단일 인스턴스.
 * 원형 프로그래스 머티리얼을 MID로 잡고 "Progress" 스칼라(0~1)를 NativeTick에서 갱신.
 * 취소 힌트(키 아이콘 + 라벨)는 자기완결 — 외부 ActionPrompt 시스템과 무관.
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class PROJECTD_API UPDCircularProgressWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="PD|HUD|Progress")
	void StartProgress(float InDuration);

	UFUNCTION(BlueprintCallable, Category="PD|HUD|Progress")
	void StopProgress();

	UFUNCTION(BlueprintCallable, Category="PD|HUD|Progress")
	void CompleteProgress();

	// 외부 push 모드 시작 — 게이지 visible, NativeTick 자동 진행 OFF.
	// 진행률은 호출자가 PushExternalProgress() 로 매번 갱신.
	UFUNCTION(BlueprintCallable, Category="PD|HUD|Progress")
	void StartExternalDriven();

	// 외부 모드일 때만 동작. Progress01 은 0~1 클램프됨.
	// OptionalRemainingSeconds 가 0 이상이면 Text_Remaining 도 갱신.
	UFUNCTION(BlueprintCallable, Category="PD|HUD|Progress")
	void PushExternalProgress(float Progress01, float OptionalRemainingSeconds = -1.f);

	UFUNCTION(BlueprintPure, Category="PD|HUD|Progress")
	bool IsRunning() const { return bRunning; }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	TObjectPtr<UImage> Image_Ring;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Remaining;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UHorizontalBox> Box_CancelHint;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UImage> Image_CancelKeyIcon;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_CancelLabel;

	// 중앙 키 아이콘 — Revive 같이 채널링 액션 키를 중앙에 표시할 때. WBP가 슬롯 두면 자동 바인딩.
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UImage> Image_CenterKeyIcon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|HUD|Progress")
	FName ProgressParamName = TEXT("Progress");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|HUD|Progress|Cancel")
	TObjectPtr<UInputAction> CancelInputAction;

	// 중앙 키 아이콘에 매핑할 InputAction. 채널링 시작 트리거 키 (예: Interact).
	// 사용자가 키를 바꾸면 EnhancedInput Subsystem 의 ControlMappingsRebuilt 이벤트로 자동 갱신.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|HUD|Progress|Center")
	TObjectPtr<UInputAction> CenterInputAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|HUD|Progress|Cancel")
	TObjectPtr<UInputMappingContext> InputMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|HUD|Progress|Cancel")
	TSoftObjectPtr<UPDKeyIconDataAsset> KeyIconMap;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|HUD|Progress|Cancel")
	FText CancelLabelText = NSLOCTEXT("PD", "CancelUseLabel", "사용취소");

private:
	UFUNCTION()
	void HandleControlMappingsRebuilt();

	void EnsureMID();
	void RefreshCancelHint();
	void RefreshCenterKeyIcon();
	void SetProgressScalar(float Value);
	void UpdateRemainingText(float RemainingSeconds);
	FKey FindKeyForAction(const UInputAction* Action) const;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> RingMID;

	float StartTime = 0.f;
	float Duration = 0.f;
	bool  bRunning = false;
	bool  bExternalDriven = false;
};