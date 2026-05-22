// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Game/PDRaidStats.h"
#include "PDPlayerRaidEntryWidget.generated.h"

class UTextBlock;
class UWidgetAnimation;

/**
 * 결산 화면의 per-player 한 줄. WBP_PDPlayerRaidEntry 부모.
 * UPDRaidEndTransitionWidget::K2_PopulateEntries 가 인스턴스 생성 후 Configure 호출.
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class PROJECTD_API UPDPlayerRaidEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 결산 데이터로 라벨 채움 + StaggerStartDelay 후 Anim_Reveal 재생. */
	UFUNCTION(BlueprintCallable, Category = "PD|Transition|Entry")
	void Configure(const FPDPlayerRaidEntryData& Data, float StaggerStartDelay);

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

private:
	FTimerHandle RevealTimerHandle;

	void PlayRevealNow();
};