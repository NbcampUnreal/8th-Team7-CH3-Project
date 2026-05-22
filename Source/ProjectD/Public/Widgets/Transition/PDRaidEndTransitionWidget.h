// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/PDActivatableBase.h"
#include "Game/PDRaidStats.h"
#include "PDRaidEndTransitionWidget.generated.h"

class UPDPlayerRaidEntryWidget;
class UPDRaidSummaryWidget;
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
	 * Entries 가 비어 있으면 GameState->PlayerArray 로 fallback 수집(이름만 채움, Stats 0).
	 * 멀티 머지 후 BuildFallbackEntries 가 PDPlayerState->Stats/bSurvived 를 그대로 채우도록 2줄만 교체.
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

	/** Configure 시점에 BP가 장식 위젯들에 액센트 컬러를 적용하도록 위임. */
	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Transition")
	void K2_ApplyAccent(FLinearColor AccentColor);

	/**
	 * Entries 해석 끝난 직후 BP에 알림. Step 4 에서 C++ 가 CreateWidget 으로 교체 예정.
	 * 그 전까지 BP 가 Box_PlayerEntries 에 EntryWidgetClass 인스턴스를 채우는 임시 위임자.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Transition")
	void K2_PopulateEntries(const TArray<FPDPlayerRaidEntryData>& Entries, float StaggerInterval);

	/** WBP_PDPlayerRaidEntry 의 base 클래스를 할당. 미할당 시 K2_PopulateEntries no-op 처리는 BP 책임. */
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
	TObjectPtr<UPDRaidSummaryWidget> Summary;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_RaidDuration;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> Box_PlayerEntries;

private:
	/** Entries 가 빈 경우 GameState->PlayerArray 로 fallback 빌드. */
	void BuildFallbackEntries(TArray<FPDPlayerRaidEntryData>& OutEntries) const;

	bool bSuccess = false;
	bool bReadyForInput = false;
	bool bInputConsumed = false;
};