// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "PDLoadingScreenSubsystem.generated.h"

class UPDLoadingScreenWidget;
class UTexture2D;
struct FWorldContext;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPDOnLoadingReasonUpdated, const FText&, NewReason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPDOnLoadingScreenHidden);

UCLASS()
class PROJECTD_API UPDLoadingScreenSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	virtual void Tick(float DeltaTime) override;
	virtual ETickableTickType GetTickableTickType() const override { return ETickableTickType::Always; }
	virtual TStatId GetStatId() const override;
	virtual bool IsTickableInEditor() const override { return false; }

	UFUNCTION(BlueprintCallable, Category = "PD|LoadingScreen")
	void SetLoadingReason(const FText& InReason);

	/** 다음 로딩 1회에만 적용되는 splash override. 풀/Static을 무시. 사용 후 자동 클리어. */
	void SetNextSplashOverride(TSoftObjectPtr<UTexture2D> InTexture);

	/** Settings.bAutoShowOnLevelLoad=false일 때, 다음 1회 트래블에만 LoadingScreen을 표시하도록 무장.
	 *  PreLoadMap 시 소비되어 자동으로 해제됨. true일 땐 호출해도 무관. */
	UFUNCTION(BlueprintCallable, Category = "PD|LoadingScreen")
	void ArmForNextTransition();

	/** PreLoadMap delegate 발화를 기다리지 않고 LoadingScreen을 즉시 viewport에 표시.
	 *  명시적 트래블 진입점에서 ServerTravel/OpenLevel 호출 직전에 부르면 "버튼 누른 즉시" 화면 전환을 보장.
	 *  HideLoadingScreen은 PostLoadMap → Hold timer 흐름으로 동일하게 처리됨. */
	UFUNCTION(BlueprintCallable, Category = "PD|LoadingScreen")
	void ShowImmediate();

	/** 로딩스크린이 현재 viewport 에 떠 있는지 (Hold 구간 포함). 트랜지션 등 후속 연출 타이밍 게이트용. */
	UFUNCTION(BlueprintPure, Category = "PD|LoadingScreen")
	bool IsLoadingScreenActive() const { return ActiveWidget != nullptr; }

	UPROPERTY(BlueprintAssignable, Category = "PD|LoadingScreen")
	FPDOnLoadingReasonUpdated OnLoadingReasonUpdated;

	// 로딩스크린이 viewport 에서 제거되는 순간 발화 (Hold 종료 → HideLoadingScreen). 후속 연출 시작 트리거.
	UPROPERTY(BlueprintAssignable, Category = "PD|LoadingScreen")
	FPDOnLoadingScreenHidden OnLoadingScreenHidden;

private:
	void HandlePreLoadMapWithContext(const FWorldContext& WorldContext, const FString& MapName);
	void HandlePostLoadMapWithWorld(UWorld* LoadedWorld);

	void ShowLoadingScreen();
	void HideLoadingScreen();
	bool ShouldDisplayInCurrentEnvironment() const;

	/** 우선순위: PendingSplashOverride > Settings.StaticSplashOverride > Pool 랜덤 > null. Pending이면 내부에서 클리어. */
	TSoftObjectPtr<UTexture2D> ResolveSplashTexture();

	UPROPERTY(Transient)
	TObjectPtr<UPDLoadingScreenWidget> ActiveWidget;

	TSoftObjectPtr<UTexture2D> PendingSplashOverride;

	FDelegateHandle PreLoadMapHandle;
	FDelegateHandle PostLoadMapHandle;

	float HoldElapsed = 0.f;
	bool bHolding = false;
	bool bArmedForNextTransition = false;

	FText CurrentReason;
};