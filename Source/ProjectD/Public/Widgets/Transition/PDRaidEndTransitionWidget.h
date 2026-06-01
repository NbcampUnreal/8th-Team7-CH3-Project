// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/PDActivatableBase.h"
#include "Game/PDRaidStats.h"
#include "PDRaidEndTransitionWidget.generated.h"

class UPDPlayerRaidEntryWidget;
class UTextBlock;
class UVerticalBox;
class UWidgetAnimation;

UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class PROJECTD_API UPDRaidEndTransitionWidget : public UPDActivatableBase
{
	GENERATED_BODY()

public:
	/**
	 * BP_PDGameMode::OnRaidEnded 에서 PushToLayer 직후 호출.
	 * Entries 가 비어 있으면 GameState->PlayerArray 로 fallback 수집 (PDPlayerState->RaidStats/IsExtracted 결합).
	 * Step 4: 위젯 인스턴싱은 C++ 가 EntryWidgetClass + CreateWidget 으로 직접 처리. BP 위임 폐기.
	 */
	UFUNCTION(BlueprintCallable, Category = "PD|Transition")
	void Configure(bool bInSuccess,
	               const TArray<FPDPlayerRaidEntryData>& Entries,
	               float RaidDurationSeconds);

protected:
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	/** Anim_IntroSequence 끝 Animation Notify에서 호출. 입력 가드 해제 + 힌트 노출. */
	UFUNCTION(BlueprintCallable, Category = "PD|Transition")
	void OnIntroFinished();

	/** Anim_BlackFade 끝 Animation Notify에서 호출. BaseLevel 트래블 발동. */
	UFUNCTION(BlueprintCallable, Category = "PD|Transition")
	void HandleTravelTrigger();

	/**
	 * Anim_IntroSequence 의 stats reveal 시점 Animation Notify 에서 호출.
	 * Configure 가 미리 생성해둔 entry 위젯들을 EntryStaggerInterval 간격으로 페이드인.
	 * BP 가 안 호출하면 entry 들은 영원히 보이지 않음 (의도) — 부재 시 LogTemp Warning.
	 */
	UFUNCTION(BlueprintCallable, Category = "PD|Transition")
	void BeginEntryReveals();

	/** Configure 시점에 BP가 장식 위젯들에 액센트 컬러를 적용하도록 위임. */
	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Transition")
	void K2_ApplyAccent(FLinearColor AccentColor);

	/** WBP_PDPlayerRaidEntry 의 base 클래스. 반드시 BP 디폴트에서 할당. 미할당 시 결산 라인 미생성. */
	UPROPERTY(EditDefaultsOnly, Category = "PD|Transition|Entries")
	TSubclassOf<UPDPlayerRaidEntryWidget> EntryWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "PD|Transition|Entries", meta = (ClampMin = "0.0"))
	float EntryStaggerInterval = 0.8f;

	UPROPERTY(EditDefaultsOnly, Category = "PD|Transition|Success")
	FLinearColor SuccessAccentColor = FLinearColor(0.357f, 0.753f, 0.922f);

	UPROPERTY(EditDefaultsOnly, Category = "PD|Transition|Success")
	FText SuccessMainText;

	UPROPERTY(EditDefaultsOnly, Category = "PD|Transition|Success")
	FText SuccessSubtitleText;

	UPROPERTY(EditDefaultsOnly, Category = "PD|Transition|Failure")
	FLinearColor FailureAccentColor = FLinearColor(0.878f, 0.471f, 0.337f);

	UPROPERTY(EditDefaultsOnly, Category = "PD|Transition|Failure")
	FText FailureMainText;

	UPROPERTY(EditDefaultsOnly, Category = "PD|Transition|Failure")
	FText FailureSubtitleText;

	UPROPERTY(Transient, BlueprintReadOnly, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> Anim_IntroSequence;

	UPROPERTY(Transient, BlueprintReadOnly, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> Anim_BlackFade;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Main;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Subtitle;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_ContinuePrompt;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_RaidDuration;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> Box_PlayerEntries;

private:
	/** Entries 가 빈 경우 GameState->PlayerArray 로 fallback 빌드. */
	void BuildFallbackEntries(TArray<FPDPlayerRaidEntryData>& OutEntries) const;

	// Configure 에서 생성한 entry 위젯 보관. BeginEntryReveals 가 인덱스 기반 stagger 로 reveal.
	UPROPERTY(Transient)
	TArray<TObjectPtr<UPDPlayerRaidEntryWidget>> EntryWidgets;

	bool bSuccess = false;
	bool bReadyForInput = false;
	bool bInputConsumed = false;
};