// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/Transition/PDPlayerRaidEntryWidget.h"

#include "Animation/WidgetAnimation.h"
#include "Components/TextBlock.h"
#include "TimerManager.h"

void UPDPlayerRaidEntryWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// reveal 전까지 안 보이도록 초기 상태 가드.
	// 디자이너가 WBP 의 Default RenderOpacity 를 0 으로 둬도 동일 효과.
	SetRenderOpacity(0.f);
}

void UPDPlayerRaidEntryWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RevealTimerHandle);
	}
	Super::NativeDestruct();
}

void UPDPlayerRaidEntryWidget::Configure(const FPDPlayerRaidEntryData& Data, float StaggerStartDelay)
{
	if (Text_PlayerName)
	{
		Text_PlayerName->SetText(FText::FromString(Data.PlayerName));
	}

	if (Text_Status)
	{
		Text_Status->SetText(Data.bSurvived ? SurvivedText : DownText);
		Text_Status->SetColorAndOpacity(FSlateColor(Data.bSurvived ? SurvivedColor : DownColor));
	}

	if (Text_KillsValue)
	{
		Text_KillsValue->SetText(FText::AsNumber(Data.Stats.Kills));
	}
	if (Text_GoldValue)
	{
		Text_GoldValue->SetText(FText::AsNumber(Data.Stats.GoldDelta));
	}
	if (Text_ItemsValue)
	{
		Text_ItemsValue->SetText(FText::AsNumber(Data.Stats.ItemDelta));
	}

	K2_OnConfigured(Data);

	// Stagger: 양수 delay 면 타이머로 지연, 0 이하면 즉시 reveal.
	UWorld* World = GetWorld();
	if (!World) return;

	World->GetTimerManager().ClearTimer(RevealTimerHandle);

	if (StaggerStartDelay > KINDA_SMALL_NUMBER)
	{
		World->GetTimerManager().SetTimer(
			RevealTimerHandle,
			this,
			&UPDPlayerRaidEntryWidget::PlayRevealNow,
			StaggerStartDelay,
			false);
	}
	else
	{
		PlayRevealNow();
	}
}

void UPDPlayerRaidEntryWidget::PlayRevealNow()
{
	SetRenderOpacity(1.f);

	if (Anim_Reveal)
	{
		PlayAnimation(Anim_Reveal);
	}
}