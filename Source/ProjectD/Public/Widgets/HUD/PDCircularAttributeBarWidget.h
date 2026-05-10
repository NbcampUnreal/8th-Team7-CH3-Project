// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/HUD/PDAttributeBarWidget.h"
#include "PDCircularAttributeBarWidget.generated.h"

class UImage;
class UTextBlock;
class UMaterialInstanceDynamic;

/**
 * 원형 ProgressBar. UImage 의 머터리얼이 fill을 마스킹하고,
 * C++는 매 변경마다 MID 의 스칼라 파라미터(Percent)에 0~1 값을 push.
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class PROJECTD_API UPDCircularAttributeBarWidget : public UPDAttributeBarWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void OnValuesUpdated(float Current, float Max, float Percent) override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UImage> Image_Ring;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Value;

	// 마스터 머터리얼의 fill 스칼라 파라미터 이름. WBP 별로 다른 머터리얼을 쓰면 변경.
	UPROPERTY(EditDefaultsOnly, Category = "PD|HUD")
	FName PercentParamName = TEXT("Percent");

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> RingMID;
};
