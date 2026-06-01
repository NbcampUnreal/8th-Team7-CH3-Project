// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/PDLoadingScreenSubsystem.h"

#include "Blueprint/UserWidget.h"
#include "DeveloperSettings/PDLoadingScreenSettings.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Texture2D.h"
#include "Widgets/Transition/PDLoadingScreenWidget.h"

void UPDLoadingScreenSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	PreLoadMapHandle = FCoreUObjectDelegates::PreLoadMapWithContext.AddUObject(
		this, &UPDLoadingScreenSubsystem::HandlePreLoadMapWithContext);
	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
		this, &UPDLoadingScreenSubsystem::HandlePostLoadMapWithWorld);
}

void UPDLoadingScreenSubsystem::Deinitialize()
{
	FCoreUObjectDelegates::PreLoadMapWithContext.Remove(PreLoadMapHandle);
	FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);

	HideLoadingScreen();

	Super::Deinitialize();
}

void UPDLoadingScreenSubsystem::Tick(float DeltaTime)
{
	if (!bHolding)
	{
		return;
	}

	HoldElapsed += DeltaTime;

	const UPDLoadingScreenSettings* Settings = GetDefault<UPDLoadingScreenSettings>();
	const float HoldSeconds = Settings ? Settings->HoldLoadingScreenExtraSeconds : 0.f;

	if (HoldElapsed >= HoldSeconds)
	{
		bHolding = false;
		HoldElapsed = 0.f;
		HideLoadingScreen();
	}
}

TStatId UPDLoadingScreenSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UPDLoadingScreenSubsystem, STATGROUP_Tickables);
}

void UPDLoadingScreenSubsystem::SetLoadingReason(const FText& InReason)
{
	CurrentReason = InReason;
	OnLoadingReasonUpdated.Broadcast(InReason);
}

void UPDLoadingScreenSubsystem::SetNextSplashOverride(TSoftObjectPtr<UTexture2D> InTexture)
{
	PendingSplashOverride = InTexture;
}

void UPDLoadingScreenSubsystem::ArmForNextTransition()
{
	bArmedForNextTransition = true;
}

void UPDLoadingScreenSubsystem::ShowImmediate()
{
	// PreLoadMap에서도 일관되게 무장 상태를 표시하기 위해 함께 set.
	// 이미 ActiveWidget이 있어서 PreLoadMap 측 ShowLoadingScreen은 early return함.
	bArmedForNextTransition = true;

	if (!ShouldDisplayInCurrentEnvironment())
	{
		return;
	}
	ShowLoadingScreen();
}

TSoftObjectPtr<UTexture2D> UPDLoadingScreenSubsystem::ResolveSplashTexture()
{
	if (!PendingSplashOverride.IsNull())
	{
		const TSoftObjectPtr<UTexture2D> Picked = PendingSplashOverride;
		PendingSplashOverride.Reset();
		return Picked;
	}

	const UPDLoadingScreenSettings* Settings = GetDefault<UPDLoadingScreenSettings>();
	if (!Settings)
	{
		return TSoftObjectPtr<UTexture2D>();
	}

	if (!Settings->StaticSplashOverride.IsNull())
	{
		return Settings->StaticSplashOverride;
	}

	const TArray<TSoftObjectPtr<UTexture2D>>& Pool = Settings->SplashImagePool;
	if (Pool.Num() > 0)
	{
		const int32 Idx = FMath::RandRange(0, Pool.Num() - 1);
		return Pool[Idx];
	}

	return TSoftObjectPtr<UTexture2D>();
}

void UPDLoadingScreenSubsystem::HandlePreLoadMapWithContext(const FWorldContext& WorldContext, const FString& MapName)
{
	if (!ShouldDisplayInCurrentEnvironment())
	{
		return;
	}

	const UPDLoadingScreenSettings* Settings = GetDefault<UPDLoadingScreenSettings>();
	const bool bAutoShow = Settings ? Settings->bAutoShowOnLevelLoad : true;
	const bool bShouldShow = bAutoShow || bArmedForNextTransition;

	// arm 신호는 1회만 소비. 다음 트래블엔 다시 arm 필요.
	bArmedForNextTransition = false;

	if (!bShouldShow)
	{
		return;
	}

	ShowLoadingScreen();
}

void UPDLoadingScreenSubsystem::HandlePostLoadMapWithWorld(UWorld* LoadedWorld)
{
	if (!ActiveWidget)
	{
		return;
	}

	bHolding = true;
	HoldElapsed = 0.f;
}

void UPDLoadingScreenSubsystem::ShowLoadingScreen()
{
	if (ActiveWidget)
	{
		return;
	}

	const UPDLoadingScreenSettings* Settings = GetDefault<UPDLoadingScreenSettings>();
	if (!Settings || Settings->SoftLoadingScreenWidgetClass.IsNull())
	{
		return;
	}

	UClass* WidgetClass = Settings->SoftLoadingScreenWidgetClass.LoadSynchronous();
	if (!WidgetClass)
	{
		return;
	}

	UGameInstance* GI = GetGameInstance();
	if (!GI) return;

	UGameViewportClient* Viewport = GI->GetGameViewportClient();
	if (!Viewport) return;

	UUserWidget* Widget = CreateWidget<UUserWidget>(GI, WidgetClass);
	if (!Widget) return;

	ActiveWidget = Cast<UPDLoadingScreenWidget>(Widget);

	Viewport->AddViewportWidgetContent(Widget->TakeWidget(), 1000);
	Viewport->bDisableWorldRendering = true;

	if (ActiveWidget)
	{
		ActiveWidget->HandleLoadingReasonUpdated(CurrentReason);
		OnLoadingReasonUpdated.AddDynamic(ActiveWidget, &UPDLoadingScreenWidget::HandleLoadingReasonUpdated);

		const TSoftObjectPtr<UTexture2D> SplashSoft = ResolveSplashTexture();
		UTexture2D* SplashTex = SplashSoft.IsNull() ? nullptr : SplashSoft.LoadSynchronous();
		ActiveWidget->SetSplashImage(SplashTex);
	}
}

void UPDLoadingScreenSubsystem::HideLoadingScreen()
{
	if (!ActiveWidget)
	{
		return;
	}

	UGameInstance* GI = GetGameInstance();
	UGameViewportClient* Viewport = GI ? GI->GetGameViewportClient() : nullptr;

	if (Viewport)
	{
		Viewport->RemoveViewportWidgetContent(ActiveWidget->TakeWidget());
		Viewport->bDisableWorldRendering = false;
	}

	OnLoadingReasonUpdated.RemoveDynamic(ActiveWidget, &UPDLoadingScreenWidget::HandleLoadingReasonUpdated);

	ActiveWidget = nullptr;

	// 로딩스크린 제거 완료 → 후속 연출(트랜지션) 시작 신호.
	OnLoadingScreenHidden.Broadcast();
}

bool UPDLoadingScreenSubsystem::ShouldDisplayInCurrentEnvironment() const
{
#if WITH_EDITOR
	if (GIsEditor)
	{
		const UPDLoadingScreenSettings* Settings = GetDefault<UPDLoadingScreenSettings>();
		return Settings && Settings->bShouldShowLoadingScreenInEditor;
	}
#endif
	return true;
}