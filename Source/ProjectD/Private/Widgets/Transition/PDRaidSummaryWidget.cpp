// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/Transition/PDRaidSummaryWidget.h"

#include "Animation/WidgetAnimation.h"
#include "Components/TextBlock.h"

void UPDRaidSummaryWidget::Configure(const FPDRaidStats& Stats)
{
	if (Text_KillsValue)
	{
		Text_KillsValue->SetText(FText::AsNumber(Stats.Kills));
		Text_KillsValue->SetColorAndOpacity(FSlateColor(PositiveValueColor));
	}

	if (Text_TimeValue)
	{
		const int32 Total = FMath::Max(0, FMath::FloorToInt(Stats.SurvivalSeconds));
		Text_TimeValue->SetText(FText::FromString(FString::Printf(TEXT("%02d:%02d"), Total / 60, Total % 60)));
		Text_TimeValue->SetColorAndOpacity(FSlateColor(PositiveValueColor));
	}

	if (Text_GoldValue)
	{
		Text_GoldValue->SetText(FText::AsNumber(Stats.GoldDelta));
		Text_GoldValue->SetColorAndOpacity(FSlateColor(Stats.GoldDelta < 0 ? NegativeValueColor : PositiveValueColor));
	}

	if (Text_ItemsValue)
	{
		Text_ItemsValue->SetText(FText::AsNumber(Stats.ItemDelta));
		Text_ItemsValue->SetColorAndOpacity(FSlateColor(Stats.ItemDelta < 0 ? NegativeValueColor : PositiveValueColor));
	}
}

void UPDRaidSummaryWidget::PlayReveal()
{
	if (Anim_Reveal)
	{
		PlayAnimation(Anim_Reveal);
	}
}