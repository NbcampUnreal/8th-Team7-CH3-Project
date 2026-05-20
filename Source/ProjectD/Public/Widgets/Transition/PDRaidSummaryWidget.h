// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Game/PDRaidStats.h"
#include "PDRaidSummaryWidget.generated.h"

class USizeBox;
class UTextBlock;
class UWidgetAnimation;

// 레이드 결산 행(처치/생존/골드/아이템) 표시. 펼침 애니는 자기 자신 안에서 보유.
UCLASS(BlueprintType, meta = (DisableNativeTick))
class PROJECTD_API UPDRaidSummaryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "PD|Raid|Summary")
	void Configure(const FPDRaidStats& Stats);

	UFUNCTION(BlueprintCallable, Category = "PD|Raid|Summary")
	void PlayReveal();

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_KillsValue;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_TimeValue;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_GoldValue;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_ItemsValue;

	UPROPERTY(Transient, BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> SBox_Reveal;

	UPROPERTY(Transient, BlueprintReadOnly, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> Anim_Reveal;

	UPROPERTY(EditDefaultsOnly, Category = "PD|Raid|Summary|Color")
	FLinearColor PositiveValueColor = FLinearColor::White;

	UPROPERTY(EditDefaultsOnly, Category = "PD|Raid|Summary|Color")
	FLinearColor NegativeValueColor = FLinearColor(0.878f, 0.471f, 0.337f);
};