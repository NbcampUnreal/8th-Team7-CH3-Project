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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|HUD|Progress")
	FName ProgressParamName = TEXT("Progress");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|HUD|Progress|Cancel")
	TObjectPtr<UInputAction> CancelInputAction;

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
	void SetProgressScalar(float Value);
	void UpdateRemainingText(float RemainingSeconds);
	FKey FindKeyForAction(const UInputAction* Action) const;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> RingMID;

	float StartTime = 0.f;
	float Duration = 0.f;
	bool  bRunning = false;
};