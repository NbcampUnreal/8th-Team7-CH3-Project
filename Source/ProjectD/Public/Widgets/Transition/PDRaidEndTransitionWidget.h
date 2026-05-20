// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/PDActivatableBase.h"
#include "Game/PDRaidStats.h"
#include "PDRaidEndTransitionWidget.generated.h"

class UPDRaidSummaryWidget;
class UTextBlock;
class UWidgetAnimation;

UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class PROJECTD_API UPDRaidEndTransitionWidget : public UPDActivatableBase
{
	GENERATED_BODY()

public:
	/** BP_PDGameMode::OnRaidEnded(bSuccess, Stats)에서 PushToLayer 직후 호출. */
	UFUNCTION(BlueprintCallable, Category = "PD|Transition")
	void Configure(bool bInSuccess, const FPDRaidStats& Stats);

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

private:
	bool bSuccess = false;
	bool bReadyForInput = false;
	bool bInputConsumed = false;
};