// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/HUD/PDLinearAttributeBarWidget.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UPDLinearAttributeBarWidget::OnValuesUpdated(float Current, float Max, float Percent)
{
	UE_LOG(LogTemp, Warning, TEXT("OnValuesUpdated: Percent=%.2f, ProgressBar is %s"), Percent, ProgressBar ? TEXT("VALID") : TEXT("NULL"));
	if (ProgressBar)
	{
		ProgressBar->SetPercent(Percent);
	}

	if (Text_Value)
	{
		Text_Value->SetText(FText::FromString(
			FString::Printf(TEXT("%s : %d / %d"),
				*TargetText,
				FMath::RoundToInt(Current),
				FMath::RoundToInt(Max))));
	}
}
