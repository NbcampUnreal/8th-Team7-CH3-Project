// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/Transition/PDRaidEndTransitionWidget.h"

#include "Animation/WidgetAnimation.h"
#include "Components/TextBlock.h"
#include "Core/PDGameInstance.h"
#include "Widgets/Transition/PDRaidSummaryWidget.h"

void UPDRaidEndTransitionWidget::Configure(bool bInSuccess, const FPDRaidStats& Stats)
{
	bSuccess = bInSuccess;

	if (Text_Main)
	{
		Text_Main->SetText(bSuccess ? SuccessMainText : FailureMainText);
		Text_Main->SetColorAndOpacity(FSlateColor(bSuccess ? SuccessAccentColor : FailureAccentColor));
	}
	if (Text_Subtitle)
	{
		Text_Subtitle->SetText(bSuccess ? SuccessSubtitleText : FailureSubtitleText);
	}
	if (Summary)
	{
		Summary->Configure(Stats);
	}

	K2_ApplyAccent(bSuccess ? SuccessAccentColor : FailureAccentColor);
}

void UPDRaidEndTransitionWidget::NativeOnActivated()
{
	Super::NativeOnActivated();

	bReadyForInput = false;
	bInputConsumed = false;

	if (Text_ContinuePrompt)
	{
		Text_ContinuePrompt->SetVisibility(ESlateVisibility::Hidden);
	}

	if (Anim_IntroSequence)
	{
		PlayAnimation(Anim_IntroSequence);
	}
	else
	{
		OnIntroFinished();
	}
}

void UPDRaidEndTransitionWidget::NativeOnDeactivated()
{
	if (Anim_IntroSequence) StopAnimation(Anim_IntroSequence);
	if (Anim_BlackFade)     StopAnimation(Anim_BlackFade);

	Super::NativeOnDeactivated();
}

FReply UPDRaidEndTransitionWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!bReadyForInput || bInputConsumed)
	{
		return FReply::Unhandled();
	}
	if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return FReply::Unhandled();
	}

	bInputConsumed = true;

	if (Anim_BlackFade)
	{
		PlayAnimation(Anim_BlackFade);
	}
	else
	{
		HandleTravelTrigger();
	}

	return FReply::Handled();
}

void UPDRaidEndTransitionWidget::OnIntroFinished()
{
	bReadyForInput = true;

	if (Text_ContinuePrompt)
	{
		Text_ContinuePrompt->SetVisibility(ESlateVisibility::Visible);
	}
}

void UPDRaidEndTransitionWidget::HandleTravelTrigger()
{
	UPDGameInstance* GI = GetGameInstance<UPDGameInstance>();
	if (!GI) return;

	GI->TravelToLevel(GI->GetBaseLevel(), /*bMarkBaseResetPending=*/false);
}