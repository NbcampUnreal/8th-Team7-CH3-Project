// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/Transition/PDRaidEndTransitionWidget.h"

#include "Animation/WidgetAnimation.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Core/PDPlayerController.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Widgets/Transition/PDRaidSummaryWidget.h"

void UPDRaidEndTransitionWidget::Configure(bool bInSuccess,
                                           const TArray<FPDPlayerRaidEntryData>& Entries,
                                           float RaidDurationSeconds)
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

	if (Text_RaidDuration)
	{
		const int32 TotalSeconds = FMath::Max(0, FMath::FloorToInt(RaidDurationSeconds));
		const int32 Minutes = TotalSeconds / 60;
		const int32 Seconds = TotalSeconds % 60;
		Text_RaidDuration->SetText(FText::FromString(FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds)));
	}

	// Entries 결정: 외부 전달이 비어 있으면 GameState->PlayerArray 로 fallback.
	TArray<FPDPlayerRaidEntryData> ResolvedEntries = Entries;
	if (ResolvedEntries.Num() == 0)
	{
		BuildFallbackEntries(ResolvedEntries);
	}

	if (Box_PlayerEntries)
	{
		Box_PlayerEntries->ClearChildren();
	}

	// Step 4 전까지 위젯 인스턴싱은 BP 위임. Step 4 들어오면 BIE 폐기 + 여기서 직접 CreateWidget.
	K2_PopulateEntries(ResolvedEntries, EntryStaggerInterval);

	// 기존 Summary 는 BindWidgetOptional. 디자이너 작업 전 시각 검증용으로 첫 Entry Stats 만 전달.
	// Step 4 에서 Summary 완전 폐기 예정.
	if (Summary)
	{
		const FPDRaidStats FirstStats = ResolvedEntries.Num() > 0
			? ResolvedEntries[0].Stats
			: FPDRaidStats{};
		Summary->Configure(FirstStats);
	}

	K2_ApplyAccent(bSuccess ? SuccessAccentColor : FailureAccentColor);
}

void UPDRaidEndTransitionWidget::BuildFallbackEntries(TArray<FPDPlayerRaidEntryData>& OutEntries) const
{
	const AGameStateBase* GS = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	if (!GS) return;

	OutEntries.Reserve(GS->PlayerArray.Num());
	for (APlayerState* PS : GS->PlayerArray)
	{
		if (!PS) continue;

		FPDPlayerRaidEntryData Data;
		Data.PlayerName = PS->GetPlayerName();
		// 머지 후: Cast<APDPlayerState>(PS) 로 받아 Stats / bSurvived 그대로 채우기.
		Data.bSurvived = false;
		Data.Stats     = FPDRaidStats{};
		OutEntries.Add(MoveTemp(Data));
	}
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
	if (APDPlayerController* PDPC = Cast<APDPlayerController>(GetOwningPlayer()))
	{
		PDPC->Server_RequestBaseTravel();
	}
}