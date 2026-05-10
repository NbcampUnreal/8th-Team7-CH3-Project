// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/HUD/PDCircularAttributeBarWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Materials/MaterialInstanceDynamic.h"

void UPDCircularAttributeBarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Image_Ring)
	{
		// GetDynamicMaterial: 이미 MID면 그대로 반환, 아니면 Brush 머터리얼로부터 생성해 박아준다.
		RingMID = Image_Ring->GetDynamicMaterial();
	}
}

void UPDCircularAttributeBarWidget::OnValuesUpdated(float Current, float Max, float Percent)
{
	if (RingMID)
	{
		RingMID->SetScalarParameterValue(PercentParamName, Percent);
	}

	if (Text_Value)
	{
		Text_Value->SetText(FText::FromString(
			FString::Printf(TEXT("%d / %d"),
				FMath::RoundToInt(Current),
				FMath::RoundToInt(Max))));
	}
}
