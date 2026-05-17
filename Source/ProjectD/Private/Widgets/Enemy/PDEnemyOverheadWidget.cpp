#include "Widgets/Enemy/PDEnemyOverheadWidget.h"

#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/Widget.h"
#include "Engine/World.h"
#include "TimerManager.h"

void UPDEnemyOverheadWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// 시작 상태: HP/아이콘 모두 collapsed — Show*() 호출 전까지 무노출.
	if (HealthRoot)  HealthRoot->SetVisibility(ESlateVisibility::Collapsed);
	if (AlertIcon)   AlertIcon->SetVisibility(ESlateVisibility::Collapsed);
	if (CombatIcon)  CombatIcon->SetVisibility(ESlateVisibility::Collapsed);
}

void UPDEnemyOverheadWidget::SetHealth(float Current, float Max)
{
	if (!HealthBar) return;
	const float Percent = (Max > KINDA_SMALL_NUMBER)
		? FMath::Clamp(Current / Max, 0.f, 1.f)
		: 0.f;
	HealthBar->SetPercent(Percent);
}

void UPDEnemyOverheadWidget::ShowHealth()
{
	if (HealthRoot)
	{
		HealthRoot->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UPDEnemyOverheadWidget::ShowSpeech(EPDEnemyState NewState, float Duration)
{
	// 직전에 표시한 state 와 동일하면 무시 — Alert↔Chase 토글 등으로 인한 ! 깜빡임 방지.
	// (Pawn::SetEnemyState 가 정말 같은 state 반복은 이미 차단하므로, 이 가드는 인접 state 토글의 백업.)
	if (LastShownState == NewState) return;
	LastShownState = NewState;

	// 상태별 아이콘 단일 노출 — Alert/Combat 외에는 둘 다 숨김.
	const bool bShowAlert  = (NewState == EPDEnemyState::Alert);
	const bool bShowCombat = (NewState == EPDEnemyState::Combat);

	if (AlertIcon)
	{
		AlertIcon->SetVisibility(bShowAlert
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}
	if (CombatIcon)
	{
		CombatIcon->SetVisibility(bShowCombat
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}

	if (UWorld* World = GetWorld())
	{
		FTimerManager& TM = World->GetTimerManager();
		TM.ClearTimer(SpeechTimerHandle);
		// 노출 대상이 있고 Duration > 0 일 때만 자동 숨김 예약.
		if ((bShowAlert || bShowCombat) && Duration > 0.f)
		{
			TM.SetTimer(SpeechTimerHandle, this,
				&UPDEnemyOverheadWidget::HandleSpeechTimeout, Duration, false);
		}
	}
}

void UPDEnemyOverheadWidget::HandleSpeechTimeout()
{
	// 노출 유효기간 종료 — 같은 state 가 다시 들어오면 정상 표시되도록 추적값 리셋.
	LastShownState = EPDEnemyState::Idle;
	if (AlertIcon)  AlertIcon->SetVisibility(ESlateVisibility::Collapsed);
	if (CombatIcon) CombatIcon->SetVisibility(ESlateVisibility::Collapsed);
}

void UPDEnemyOverheadWidget::HideAll()
{
	LastShownState = EPDEnemyState::Idle;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SpeechTimerHandle);
	}
	if (HealthRoot) HealthRoot->SetVisibility(ESlateVisibility::Collapsed);
	if (AlertIcon)  AlertIcon->SetVisibility(ESlateVisibility::Collapsed);
	if (CombatIcon) CombatIcon->SetVisibility(ESlateVisibility::Collapsed);
}
