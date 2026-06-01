// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/PDActivatableBase.h"
#include "PDOptionsScreenWidget.generated.h"

class UButton;
class UComboBoxString;
class UCheckBox;
class USlider;

/**
 * 옵션 화면. UGameUserSettings(=UPDGameUserSettings) 기반.
 * Video(해상도/화면모드/품질/VSync/감마) + Audio(볼륨/배경음/HDR) + Gameplay(난이도)를
 * 콤보·슬라이더·체크로 노출하고 Apply 시 일괄 적용·저장한다.
 * Video 핵심 4개는 BindWidget(필수), 나머지는 BindWidgetOptional(BP 점진 배치 가능).
 * UI 프레임워크는 ProjectD 자체(PDActivatableBase) — 참조 FrontendUI의 CommonUI 위젯은 사용하지 않음.
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class PROJECTD_API UPDOptionsScreenWidget : public UPDActivatableBase
{
	GENERATED_BODY()

protected:
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	virtual UWidget* GetDesiredFocusTarget_Implementation() const override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UComboBoxString> ComboBox_Resolution;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UComboBoxString> ComboBox_WindowMode;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UComboBoxString> ComboBox_Quality;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UCheckBox> CheckBox_VSync;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<USlider> Slider_Gamma;

	// ───── Audio ─────
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<USlider> Slider_OverallVolume;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<USlider> Slider_MusicVolume;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<USlider> Slider_SoundFXVolume;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UCheckBox> CheckBox_BackgroundAudio;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UCheckBox> CheckBox_HDRAudio;

	// ───── Gameplay ─────
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> ComboBox_Difficulty;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> Button_Apply;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Back;

private:
	UFUNCTION() void HandleApplyClicked();
	UFUNCTION() void HandleBackClicked();

	void PopulateOptions();
	void LoadCurrentSettings();

	TArray<FIntPoint> SupportedResolutions;
};