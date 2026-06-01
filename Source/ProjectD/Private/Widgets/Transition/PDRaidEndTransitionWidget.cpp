// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/Transition/PDRaidEndTransitionWidget.h"

#include "Animation/WidgetAnimation.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Core/PDPlayerController.h"
#include "Core/PDPlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Widgets/Transition/PDPlayerRaidEntryWidget.h"

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
		Text_RaidDuration->SetText(FText::FromString(FString::Printf(TEXT("RAID TIME  %02d:%02d"), Minutes, Seconds)));
	}

	// Entries 결정: 외부 전달이 비어 있으면 GameState->PlayerArray 로 fallback.
	TArray<FPDPlayerRaidEntryData> ResolvedEntries = Entries;
	if (ResolvedEntries.Num() == 0)
	{
		BuildFallbackEntries(ResolvedEntries);
	}

	// Step 4: C++ 가 직접 EntryWidgetClass 로 인스턴싱 + Configure. BP 위임(K2_PopulateEntries) 폐기.
	// Reveal 은 별도 — Anim_IntroSequence 의 stats reveal 노티파이가 BeginEntryReveals 호출 시점에 발생.
	EntryWidgets.Reset();

	// PS 이름으로 lookup map 구축 — entry 가 ACK 추적용 PS 받음.
	TMap<FString, APDPlayerState*> PSByName;
	if (const AGameStateBase* GSForLookup = GetWorld() ? GetWorld()->GetGameState() : nullptr)
	{
		for (APlayerState* PS : GSForLookup->PlayerArray)
		{
			if (APDPlayerState* PDPS = Cast<APDPlayerState>(PS))
			{
				PSByName.Add(PS->GetPlayerName(), PDPS);
			}
		}
	}

	if (Box_PlayerEntries)
	{
		Box_PlayerEntries->ClearChildren();

		if (!EntryWidgetClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("PDRaidEndTransitionWidget: EntryWidgetClass 미할당 — 결산 라인 미생성."));
		}
		else
		{
			APlayerController* OwningPC = GetOwningPlayer();
			for (int32 Index = 0; Index < ResolvedEntries.Num(); ++Index)
			{
				UPDPlayerRaidEntryWidget* Entry = CreateWidget<UPDPlayerRaidEntryWidget>(OwningPC, EntryWidgetClass);
				if (!Entry) continue;

				Box_PlayerEntries->AddChildToVerticalBox(Entry);
				APDPlayerState* MatchedPS = PSByName.FindRef(ResolvedEntries[Index].PlayerName);
				Entry->Configure(ResolvedEntries[Index], MatchedPS);
				EntryWidgets.Add(Entry);
			}
		}
	}

	K2_ApplyAccent(bSuccess ? SuccessAccentColor : FailureAccentColor);
}

void UPDRaidEndTransitionWidget::BeginEntryReveals()
{
	if (EntryWidgets.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("PDRaidEndTransitionWidget::BeginEntryReveals: EntryWidgets 비어있음 (Configure 미호출 또는 entry 0개)."));
		return;
	}

	for (int32 Index = 0; Index < EntryWidgets.Num(); ++Index)
	{
		UPDPlayerRaidEntryWidget* Entry = EntryWidgets[Index];
		if (!Entry) continue;
		const float Delay = Index * EntryStaggerInterval;
		Entry->PlayRevealAfter(Delay);
	}
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

		// Step 2-C: PDPlayerState 의 RaidStats / IsExtracted 를 실값으로 결합.
		if (const APDPlayerState* PDPS = Cast<APDPlayerState>(PS))
		{
			Data.bSurvived = PDPS->IsExtracted();
			Data.Stats     = PDPS->GetRaidStats();
		}
		else
		{
			Data.bSurvived = false;
			Data.Stats     = FPDRaidStats{};
		}

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
