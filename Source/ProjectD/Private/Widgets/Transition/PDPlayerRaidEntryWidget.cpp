// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/Transition/PDPlayerRaidEntryWidget.h"

#include "Animation/WidgetAnimation.h"
#include "Components/TextBlock.h"
#include "Core/PDPlayerState.h"
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

	// PS 델리게이트 구독 해제.
	if (APDPlayerState* PS = BoundPlayerState.Get())
	{
		PS->OnTravelReadyChanged.RemoveDynamic(this, &UPDPlayerRaidEntryWidget::HandleTravelReadyChanged);
	}
	BoundPlayerState = nullptr;

	Super::NativeDestruct();
}

void UPDPlayerRaidEntryWidget::Configure(const FPDPlayerRaidEntryData& Data, APDPlayerState* InPlayerState)
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
	if (Text_SurvivalValue)
	{
		Text_SurvivalValue->SetText(FormatSurvivalSeconds(Data.Stats.SurvivalSeconds));
	}

	// 새 PS 바인딩 (있다면) — 기존 구독 해제 후 재바인딩.
	if (APDPlayerState* OldPS = BoundPlayerState.Get())
	{
		OldPS->OnTravelReadyChanged.RemoveDynamic(this, &UPDPlayerRaidEntryWidget::HandleTravelReadyChanged);
	}
	BoundPlayerState = InPlayerState;
	if (InPlayerState)
	{
		InPlayerState->OnTravelReadyChanged.AddDynamic(this, &UPDPlayerRaidEntryWidget::HandleTravelReadyChanged);
	}
	RefreshAckStatus();

	K2_OnConfigured(Data);
}

void UPDPlayerRaidEntryWidget::HandleTravelReadyChanged(bool /*bIsTravelReady*/)
{
	RefreshAckStatus();
}

void UPDPlayerRaidEntryWidget::RefreshAckStatus()
{
	if (!Text_AckStatus) return;

	const APDPlayerState* PS = BoundPlayerState.Get();
	const bool bReady = PS && PS->IsTravelReady();

	Text_AckStatus->SetText(bReady ? AckReadyText : AckPendingText);
	Text_AckStatus->SetColorAndOpacity(FSlateColor(bReady ? AckReadyColor : AckPendingColor));
}

void UPDPlayerRaidEntryWidget::PlayRevealAfter(float StaggerStartDelay)
{
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

FText UPDPlayerRaidEntryWidget::FormatSurvivalSeconds(float Seconds)
{
	const int32 TotalSeconds = FMath::Max(0, FMath::FloorToInt(Seconds));
	const int32 Minutes = TotalSeconds / 60;
	const int32 RemainSeconds = TotalSeconds % 60;
	return FText::FromString(FString::Printf(TEXT("%02d:%02d"), Minutes, RemainSeconds));
}