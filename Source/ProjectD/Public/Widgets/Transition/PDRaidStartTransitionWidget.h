// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/PDActivatableBase.h"
#include "PDRaidStartTransitionWidget.generated.h"

class UTextBlock;
class UWidgetAnimation;

/**
 * Base→Raid 진입 연출 (RaidEnd 의 거울상). 라이드 맵 도착 후 StartRaid 시점에
 * 각 클라이언트 Modal 레이어에 push. "검정→시안 와이어프레임 조립→맵 재조립→HUD 핸드오프"
 * 를 위젯 애니메이션(Anim_Assembly)으로 표현. 애니메이션 끝나면 스스로 pop.
 *
 * 탑뷰 익스트랙션 각색: 레퍼런스(FPS Match-Start)의 카메라 무빙/모드 배지/점령 배너는 배제.
 * 시각 어휘(시안 와이어프레임/스캔라인/페이드)와 비트 구조만 차용. 에셋은 RaidEnd 와 공유.
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class PROJECTD_API UPDRaidStartTransitionWidget : public UPDActivatableBase
{
	GENERATED_BODY()

public:
	UPDRaidStartTransitionWidget();

	/** 서버 StartRaid 가 클라마다 push 직후 호출. 구역 이름 표시 + 조립 애니메이션 시작. */
	UFUNCTION(BlueprintCallable, Category = "PD|Transition")
	void Configure(const FText& InZoneName);

protected:
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;

	/** Anim_Assembly 끝 Animation Notify(Event Track)에서 호출. 위젯 pop + 게임플레이 핸드오프. */
	UFUNCTION(BlueprintCallable, Category = "PD|Transition")
	void HandleAssemblyFinished();

	/** Configure 시점 BP 후처리 훅 (예: 구역별 색/아이콘 스왑). */
	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Transition")
	void K2_OnConfigured(const FText& ZoneName);

	// "검정→와이어프레임→맵 재조립→HUD" 전체 조립 시퀀스. 끝 Event Track 이 HandleAssemblyFinished 호출.
	UPROPERTY(Transient, BlueprintReadOnly, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> Anim_Assembly;

	// 구역/맵 이름. WBP 가 텍스트박스 두면 표시, 없으면 무시. 비워두면 숨김(BP 재량).
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_ZoneName;

	// Anim_Assembly 미할당 시 자동 dismiss 까지 fallback. 애니메이션 있으면 Event Track 이 우선.
	UPROPERTY(EditDefaultsOnly, Category = "PD|Transition", meta = (ClampMin = "0.1", ForceUnits = "s"))
	float FallbackAutoDismissSeconds = 2.5f;

	// 로딩스크린이 내려가는 순간 호출 — 조립 애니메이션 시작. 그 전엔 검정(디자이너 디폴트) 유지.
	UFUNCTION()
	void HandleLoadingScreenHidden();

private:
	void BeginAssembly();
	void DismissSelf();

	FTimerHandle FallbackDismissHandle;
	bool bDismissed = false;
	bool bAssemblyStarted = false;
};