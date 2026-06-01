// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/Transition/PDRaidStartTransitionWidget.h"

#include "Animation/WidgetAnimation.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "Subsystems/PDFrontendUISubsystem.h"
#include "Subsystems/PDLoadingScreenSubsystem.h"
#include "TimerManager.h"

UPDRaidStartTransitionWidget::UPDRaidStartTransitionWidget()
{
	// 연출 중 게임 입력 차단 + 커서 숨김. ESC 로 닫히지 않게.
	InputMode = EWidgetInputMode::Cinematic;
	bShowMouseCursor = false;
	bAllowEscapeDismiss = false;
}

void UPDRaidStartTransitionWidget::Configure(const FText& InZoneName)
{
	if (Text_ZoneName)
	{
		Text_ZoneName->SetText(InZoneName);
		Text_ZoneName->SetVisibility(InZoneName.IsEmpty()
			? ESlateVisibility::Collapsed
			: ESlateVisibility::HitTestInvisible);
	}

	K2_OnConfigured(InZoneName);
}

void UPDRaidStartTransitionWidget::NativeOnActivated()
{
	Super::NativeOnActivated();

	bDismissed = false;
	bAssemblyStarted = false;

	// 로딩스크린이 아직 떠 있으면: 검정(디자이너 디폴트 Image_BlackFade alpha 1)을 그 밑에 미리 깔아두고
	// 로딩스크린이 내려가는 순간(OnLoadingScreenHidden) 조립 시작 → raw 맵 노출 프레임 0.
	UPDLoadingScreenSubsystem* LoadingSubsystem = nullptr;
	if (UGameInstance* GI = GetGameInstance())
	{
		LoadingSubsystem = GI->GetSubsystem<UPDLoadingScreenSubsystem>();
	}

	if (LoadingSubsystem && LoadingSubsystem->IsLoadingScreenActive())
	{
		LoadingSubsystem->OnLoadingScreenHidden.AddDynamic(this, &UPDRaidStartTransitionWidget::HandleLoadingScreenHidden);
		// 애니메이션 미시작 — 검정 유지하며 로딩스크린 내려갈 때까지 대기.
	}
	else
	{
		BeginAssembly();
	}
}

void UPDRaidStartTransitionWidget::HandleLoadingScreenHidden()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UPDLoadingScreenSubsystem* LoadingSubsystem = GI->GetSubsystem<UPDLoadingScreenSubsystem>())
		{
			LoadingSubsystem->OnLoadingScreenHidden.RemoveDynamic(this, &UPDRaidStartTransitionWidget::HandleLoadingScreenHidden);
		}
	}
	BeginAssembly();
}

void UPDRaidStartTransitionWidget::BeginAssembly()
{
	if (bAssemblyStarted) return;
	bAssemblyStarted = true;

	if (Anim_Assembly)
	{
		PlayAnimation(Anim_Assembly);
		// 안전망: Event Track 미연결이어도 애니메이션 길이만큼 뒤 자동 dismiss.
		if (UWorld* World = GetWorld())
		{
			const float Dur = FMath::Max(Anim_Assembly->GetEndTime(), FallbackAutoDismissSeconds);
			World->GetTimerManager().SetTimer(
				FallbackDismissHandle, this, &UPDRaidStartTransitionWidget::HandleAssemblyFinished, Dur, false);
		}
	}
	else if (UWorld* World = GetWorld())
	{
		// 애니메이션 없으면 fallback 시간 후 dismiss.
		World->GetTimerManager().SetTimer(
			FallbackDismissHandle, this, &UPDRaidStartTransitionWidget::HandleAssemblyFinished, FallbackAutoDismissSeconds, false);
	}
}

void UPDRaidStartTransitionWidget::NativeOnDeactivated()
{
	if (Anim_Assembly) StopAnimation(Anim_Assembly);
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FallbackDismissHandle);
	}
	// 혹시 남아있을 수 있는 구독 해제.
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UPDLoadingScreenSubsystem* LoadingSubsystem = GI->GetSubsystem<UPDLoadingScreenSubsystem>())
		{
			LoadingSubsystem->OnLoadingScreenHidden.RemoveDynamic(this, &UPDRaidStartTransitionWidget::HandleLoadingScreenHidden);
		}
	}
	Super::NativeOnDeactivated();
}

void UPDRaidStartTransitionWidget::HandleAssemblyFinished()
{
	DismissSelf();
}

void UPDRaidStartTransitionWidget::DismissSelf()
{
	if (bDismissed) return;
	bDismissed = true;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FallbackDismissHandle);
	}

	if (UPDFrontendUISubsystem* Subsystem = UPDFrontendUISubsystem::Get(this))
	{
		Subsystem->PopFromLayer(EUILayer::Modal);
	}
}