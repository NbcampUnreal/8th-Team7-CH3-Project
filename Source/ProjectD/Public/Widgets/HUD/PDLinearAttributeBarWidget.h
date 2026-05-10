// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/HUD/PDAttributeBarWidget.h"
#include "PDLinearAttributeBarWidget.generated.h"

class UProgressBar;
class UTextBlock;

/**
 * 선형 ProgressBar 기반 표시. UMG의 UProgressBar 슬레이트가 픽셀 분할을 처리.
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class PROJECTD_API UPDLinearAttributeBarWidget : public UPDAttributeBarWidget
{
	GENERATED_BODY()

protected:
	virtual void OnValuesUpdated(float Current, float Max, float Percent) override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UProgressBar> ProgressBar;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Value;
};
