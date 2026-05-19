// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/Transition/PDLoadingScreenWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

void UPDLoadingScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Anim_SpinnerRotate)
	{
		PlayAnimation(Anim_SpinnerRotate, 0.f, 0);
	}
}

void UPDLoadingScreenWidget::NativeDestruct()
{
	if (Anim_SpinnerRotate && IsAnimationPlaying(Anim_SpinnerRotate))
	{
		StopAnimation(Anim_SpinnerRotate);
	}

	Super::NativeDestruct();
}

void UPDLoadingScreenWidget::HandleLoadingReasonUpdated(const FText& InReason)
{
	if (Text_LoadingReason)
	{
		Text_LoadingReason->SetText(InReason);
	}
}

void UPDLoadingScreenWidget::SetSplashImage(UTexture2D* InTexture)
{
	if (!Image_Splash)
	{
		return;
	}

	if (InTexture)
	{
		Image_Splash->SetBrushFromTexture(InTexture);
		Image_Splash->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else
	{
		Image_Splash->SetVisibility(ESlateVisibility::Collapsed);
	}
}