// Fill out your copyright notice in the Description page of Project Settings.

#include "Widgets/HUD/PDZoneCountdownWidget.h"
#include "Components/TextBlock.h"
#include "Core/PDGameState.h"
#include "TimerManager.h"
#include "Engine/World.h"

void UPDZoneCountdownWidget::NativeConstruct()
{
	Super::NativeConstruct();

	UE_LOG(LogTemp, Warning, TEXT("[Diag-Zone] Widget NativeConstruct (this=%s)"), *GetName());

	// 시작은 비활성 → 위젯 자신을 숨김.
	bWasActive = false;
	SetVisibility(ESlateVisibility::Collapsed);
	OnCountdownVisibilityChanged(false);

	// 폴링은 타이머로. NativeTick 은 위젯이 Collapsed 면 호출되지 않아(데드락),
	// 자기 자신을 Collapse 한 채로도 GameState 변화를 감지하려면 틱과 무관한 타이머가 필요.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(RefreshTimerHandle, this, &UPDZoneCountdownWidget::Refresh, 0.1f, true);
	}

	Refresh();
}

void UPDZoneCountdownWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RefreshTimerHandle);
	}
	Super::NativeDestruct();
}

void UPDZoneCountdownWidget::Refresh()
{
	const APDGameState* GS = GetPDGameState();
	const bool bActive = GS && GS->IsZoneCountdownActive();

	if (bActive != bWasActive)
	{
		bWasActive = bActive;
		UE_LOG(LogTemp, Warning, TEXT("[Diag-Zone] Widget: active=%d (GS=%s, Type=%d, InZone=%d/%d)"),
			bActive ? 1 : 0, GS ? TEXT("ok") : TEXT("NULL"),
			GS ? static_cast<int32>(GS->GetZoneTravelType()) : -1,
			GS ? GS->GetZonePlayersInZone() : -1,
			GS ? GS->GetZoneTotalParticipants() : -1);
		// 자기 자신 표시/숨김(입력 안 막는 HitTestInvisible). 타이머 폴링이라 Collapse 해도 계속 갱신됨.
		SetVisibility(bActive ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		OnCountdownVisibilityChanged(bActive);
	}

	if (!bActive)
	{
		return;
	}

	const int32 InZone = GS->GetZonePlayersInZone();
	const int32 Total = GS->GetZoneTotalParticipants();
	const int32 RemainingSeconds = FMath::Max(0, FMath::CeilToInt(GS->GetZoneCountdownRemaining()));
	const bool bFinal = GS->IsZoneFinalCountdown();

	if (Text_ZoneCount)
	{
		Text_ZoneCount->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), InZone, Total)));
	}
	if (Text_Countdown)
	{
		Text_Countdown->SetText(FText::AsNumber(RemainingSeconds));
	}

	OnCountdownUpdated(InZone, Total, RemainingSeconds, bFinal);
}

APDGameState* UPDZoneCountdownWidget::GetPDGameState() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetGameState<APDGameState>() : nullptr;
}