#include "Widgets/HUD/PDQuipToastWidget.h"

#include "Animation/WidgetAnimation.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Data/PDQuipDataAsset.h"
#include "Engine/World.h"
#include "Subsystems/PDQuipSubsystem.h"
#include "TimerManager.h"

void UPDQuipToastWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 초기 가시 상태: 자리는 차지하되 Border_BG 단위로 보이지 않음. 애니메이션이 Border_BG의 opacity 0→1을 다룸.
	// 루트에 SetRenderOpacity(0) 걸면 자식 애니메이션이 multiplication에 막혀 영구 invisible 되므로 Border_BG에만 적용.
	SetVisibility(ESlateVisibility::HitTestInvisible);
	if (Border_BG)
	{
		Border_BG->SetRenderOpacity(0.f);
	}

	Phase = EQuipPhase::Idle;
	bHasPending = false;

	if (UPDQuipSubsystem* Sub = UPDQuipSubsystem::Get(this))
	{
		CachedSubsystem = Sub;
		QuipFiredHandle = Sub->OnQuipFired.AddUObject(this, &UPDQuipToastWidget::HandleQuipFired);
	}

	if (Anim_Enter)
	{
		FWidgetAnimationDynamicEvent EnterEvent;
		EnterEvent.BindDynamic(this, &UPDQuipToastWidget::OnEnterAnimFinishedHandler);
		BindToAnimationFinished(Anim_Enter, EnterEvent);
	}
	if (Anim_Exit)
	{
		FWidgetAnimationDynamicEvent ExitEvent;
		ExitEvent.BindDynamic(this, &UPDQuipToastWidget::OnExitAnimFinishedHandler);
		BindToAnimationFinished(Anim_Exit, ExitEvent);
	}
}

void UPDQuipToastWidget::NativeDestruct()
{
	if (UPDQuipSubsystem* Sub = CachedSubsystem.Get())
	{
		Sub->OnQuipFired.Remove(QuipFiredHandle);
	}
	QuipFiredHandle.Reset();
	CachedSubsystem.Reset();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HoldTimerHandle);
	}

	if (Anim_Enter && IsAnimationPlaying(Anim_Enter))
	{
		StopAnimation(Anim_Enter);
	}
	if (Anim_Exit && IsAnimationPlaying(Anim_Exit))
	{
		StopAnimation(Anim_Exit);
	}

	Super::NativeDestruct();
}

void UPDQuipToastWidget::HandleQuipFired(const FPDQuipEntry& Entry, const FText& Line)
{
	ShowLine(Line, Entry.DisplaySeconds);
}

void UPDQuipToastWidget::ShowLine(const FText& Line, float DisplaySeconds)
{
	const float SafeDuration = (DisplaySeconds > 0.f) ? DisplaySeconds : 1.7f;

	if (Phase == EQuipPhase::Idle)
	{
		StartShow(Line, SafeDuration);
		return;
	}

	// Soft replace: 현 라인 정상 종료 후 신규 라인 진입
	PendingLine = Line;
	PendingDisplaySeconds = SafeDuration;
	bHasPending = true;

	if (Phase == EQuipPhase::Exiting)
	{
		// 이미 Exit 중 — OnExitAnimFinishedHandler가 pending 처리
		return;
	}

	// Entering/Holding 중단 → Exit 시작
	if (Anim_Enter && IsAnimationPlaying(Anim_Enter))
	{
		StopAnimation(Anim_Enter);
	}
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HoldTimerHandle);
	}

	Phase = EQuipPhase::Exiting;
	if (Anim_Exit)
	{
		PlayAnimationForward(Anim_Exit);
	}
	else
	{
		OnExitAnimFinishedHandler();
	}
}

void UPDQuipToastWidget::StartShow(const FText& Line, float DisplaySeconds)
{
	if (Text_Line)
	{
		Text_Line->SetText(Line);
	}

	CurrentDisplaySeconds = DisplaySeconds;
	Phase = EQuipPhase::Entering;

	if (Anim_Enter)
	{
		PlayAnimationForward(Anim_Enter);
	}
	else
	{
		// Fallback: 애니 없으면 즉시 가시화 후 홀드로 전환
		if (Border_BG)
		{
			Border_BG->SetRenderOpacity(1.f);
		}
		OnEnterAnimFinishedHandler();
	}
}

void UPDQuipToastWidget::OnEnterAnimFinishedHandler()
{
	Phase = EQuipPhase::Holding;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			HoldTimerHandle,
			this,
			&UPDQuipToastWidget::StartExit,
			FMath::Max(0.01f, CurrentDisplaySeconds),
			false);
	}
}

void UPDQuipToastWidget::StartExit()
{
	Phase = EQuipPhase::Exiting;

	if (Anim_Exit)
	{
		PlayAnimationForward(Anim_Exit);
	}
	else
	{
		if (Border_BG)
		{
			Border_BG->SetRenderOpacity(0.f);
		}
		OnExitAnimFinishedHandler();
	}
}

void UPDQuipToastWidget::OnExitAnimFinishedHandler()
{
	if (bHasPending)
	{
		bHasPending = false;
		const FText Next = PendingLine;
		const float NextDur = PendingDisplaySeconds;
		PendingLine = FText::GetEmpty();
		PendingDisplaySeconds = 0.f;
		StartShow(Next, NextDur);
	}
	else
	{
		Phase = EQuipPhase::Idle;
	}
}