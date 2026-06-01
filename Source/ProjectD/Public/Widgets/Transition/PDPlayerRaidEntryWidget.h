// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Game/PDRaidStats.h"
#include "PDPlayerRaidEntryWidget.generated.h"

class UTextBlock;
class UWidgetAnimation;
class APDPlayerState;

/**
 * 결산 화면의 per-player 한 줄. WBP_PDPlayerRaidEntry 부모.
 * UPDRaidEndTransitionWidget::Configure 가 C++ CreateWidget 으로 인스턴스 생성 후 Configure 호출 (Step 4).
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class PROJECTD_API UPDPlayerRaidEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 결산 데이터로 라벨만 채움. Reveal 은 PlayRevealAfter 가 별도 호출.
	 *  InPlayerState 는 ACK(좌클릭) 상태 실시간 추적용 — null 이면 ACK 컬럼 미표시. */
	UFUNCTION(BlueprintCallable, Category = "PD|Transition|Entry")
	void Configure(const FPDPlayerRaidEntryData& Data, APDPlayerState* InPlayerState);

	/** StaggerStartDelay 초 후 Anim_Reveal 재생. 부모 위젯이 stagger 인덱스 곱해 호출. */
	UFUNCTION(BlueprintCallable, Category = "PD|Transition|Entry")
	void PlayRevealAfter(float StaggerStartDelay);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

	/** Configure 끝에 BP 후처리 훅. (예: 본인 PC 강조, 아이콘 스왑 등) */
	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Transition|Entry")
	void K2_OnConfigured(const FPDPlayerRaidEntryData& Data);

	UPROPERTY(EditDefaultsOnly, Category = "PD|Transition|Entry|Status")
	FText SurvivedText;

	UPROPERTY(EditDefaultsOnly, Category = "PD|Transition|Entry|Status")
	FText DownText;

	UPROPERTY(EditDefaultsOnly, Category = "PD|Transition|Entry|Status")
	FLinearColor SurvivedColor = FLinearColor(0.357f, 0.753f, 0.922f);

	UPROPERTY(EditDefaultsOnly, Category = "PD|Transition|Entry|Status")
	FLinearColor DownColor = FLinearColor(0.878f, 0.471f, 0.337f);

	UPROPERTY(Transient, BlueprintReadOnly, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> Anim_Reveal;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_PlayerName;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Status;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_KillsValue;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_GoldValue;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_ItemsValue;

	// 생존 시간 MM:SS. WBP가 텍스트박스를 두면 자동 바인딩, 없으면 무시.
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_SurvivalValue;

	// 결산 위젯 ACK 상태 (좌클릭 여부). WBP 에 텍스트박스 두면 ✓/⏳ 표시, 없으면 무시.
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_AckStatus;

	UPROPERTY(EditDefaultsOnly, Category = "PD|Transition|Entry|Ack")
	FText AckReadyText;

	UPROPERTY(EditDefaultsOnly, Category = "PD|Transition|Entry|Ack")
	FText AckPendingText;

	UPROPERTY(EditDefaultsOnly, Category = "PD|Transition|Entry|Ack")
	FLinearColor AckReadyColor = FLinearColor(0.357f, 0.753f, 0.922f);

	UPROPERTY(EditDefaultsOnly, Category = "PD|Transition|Entry|Ack")
	FLinearColor AckPendingColor = FLinearColor(0.7f, 0.7f, 0.7f);

private:
	FTimerHandle RevealTimerHandle;

	void PlayRevealNow();

	static FText FormatSurvivalSeconds(float Seconds);

	// 결산 ACK 추적용 — Configure 가 PS 바인딩, NativeDestruct 가 해제.
	UPROPERTY(Transient)
	TWeakObjectPtr<APDPlayerState> BoundPlayerState;

	UFUNCTION()
	void HandleTravelReadyChanged(bool bIsTravelReady);

	void RefreshAckStatus();
};