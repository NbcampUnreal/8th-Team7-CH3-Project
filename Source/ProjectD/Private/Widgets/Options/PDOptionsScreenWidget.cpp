// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/Options/PDOptionsScreenWidget.h"

#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/Slider.h"
#include "Core/PDGameUserSettings.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Subsystems/PDFrontendUISubsystem.h"
#include "Type/Types.h"

void UPDOptionsScreenWidget::NativeOnActivated()
{
	Super::NativeOnActivated();

	if (Button_Apply)
	{
		Button_Apply->OnClicked.AddDynamic(this, &UPDOptionsScreenWidget::HandleApplyClicked);
	}
	if (Button_Back)
	{
		Button_Back->OnClicked.AddDynamic(this, &UPDOptionsScreenWidget::HandleBackClicked);
	}

	PopulateOptions();
	LoadCurrentSettings();
}

void UPDOptionsScreenWidget::NativeOnDeactivated()
{
	if (Button_Apply)
	{
		Button_Apply->OnClicked.RemoveDynamic(this, &UPDOptionsScreenWidget::HandleApplyClicked);
	}
	if (Button_Back)
	{
		Button_Back->OnClicked.RemoveDynamic(this, &UPDOptionsScreenWidget::HandleBackClicked);
	}

	Super::NativeOnDeactivated();
}

UWidget* UPDOptionsScreenWidget::GetDesiredFocusTarget_Implementation() const
{
	return Button_Apply;
}

void UPDOptionsScreenWidget::PopulateOptions()
{
	if (ComboBox_WindowMode)
	{
		ComboBox_WindowMode->ClearOptions();
		// EWindowMode 순서와 일치: Fullscreen=0, WindowedFullscreen=1, Windowed=2
		ComboBox_WindowMode->AddOption(TEXT("Fullscreen"));
		ComboBox_WindowMode->AddOption(TEXT("Windowed Fullscreen"));
		ComboBox_WindowMode->AddOption(TEXT("Windowed"));
	}

	if (ComboBox_Quality)
	{
		ComboBox_Quality->ClearOptions();
		// OverallScalabilityLevel 0~3
		ComboBox_Quality->AddOption(TEXT("Low"));
		ComboBox_Quality->AddOption(TEXT("Medium"));
		ComboBox_Quality->AddOption(TEXT("High"));
		ComboBox_Quality->AddOption(TEXT("Epic"));
	}

	if (ComboBox_Resolution)
	{
		ComboBox_Resolution->ClearOptions();
		SupportedResolutions.Reset();
		UKismetSystemLibrary::GetSupportedFullscreenResolutions(SupportedResolutions);
		for (const FIntPoint& Res : SupportedResolutions)
		{
			ComboBox_Resolution->AddOption(FString::Printf(TEXT("%d x %d"), Res.X, Res.Y));
		}
	}

	if (ComboBox_Difficulty)
	{
		ComboBox_Difficulty->ClearOptions();
		ComboBox_Difficulty->AddOption(TEXT("Easy"));
		ComboBox_Difficulty->AddOption(TEXT("Normal"));
		ComboBox_Difficulty->AddOption(TEXT("Hard"));
	}
}

void UPDOptionsScreenWidget::LoadCurrentSettings()
{
	UPDGameUserSettings* Settings = UPDGameUserSettings::Get();
	if (!Settings)
	{
		return;
	}

	if (ComboBox_WindowMode)
	{
		const int32 ModeIdx = static_cast<int32>(Settings->GetFullscreenMode());
		ComboBox_WindowMode->SetSelectedIndex(FMath::Clamp(ModeIdx, 0, 2));
	}

	if (ComboBox_Quality)
	{
		const int32 Quality = Settings->GetOverallScalabilityLevel();
		// -1(커스텀 혼합)이면 High로 표시.
		ComboBox_Quality->SetSelectedIndex(Quality >= 0 ? FMath::Clamp(Quality, 0, 3) : 2);
	}

	if (ComboBox_Resolution)
	{
		const FIntPoint Current = Settings->GetScreenResolution();
		const int32 Idx = SupportedResolutions.IndexOfByKey(Current);
		if (Idx != INDEX_NONE)
		{
			ComboBox_Resolution->SetSelectedIndex(Idx);
		}
	}

	if (CheckBox_VSync)
	{
		CheckBox_VSync->SetIsChecked(Settings->IsVSyncEnabled());
	}

	if (Slider_Gamma)
	{
		Slider_Gamma->SetValue(Settings->GetCurrentDisplayGamma());
	}

	if (Slider_OverallVolume)
	{
		Slider_OverallVolume->SetValue(Settings->GetOverallVolume());
	}
	if (Slider_MusicVolume)
	{
		Slider_MusicVolume->SetValue(Settings->GetMusicVolume());
	}
	if (Slider_SoundFXVolume)
	{
		Slider_SoundFXVolume->SetValue(Settings->GetSoundFXVolume());
	}
	if (CheckBox_BackgroundAudio)
	{
		CheckBox_BackgroundAudio->SetIsChecked(Settings->GetAllowBackgroundAudio());
	}
	if (CheckBox_HDRAudio)
	{
		CheckBox_HDRAudio->SetIsChecked(Settings->GetUseHDRAudioMode());
	}

	if (ComboBox_Difficulty)
	{
		const int32 DiffIdx = ComboBox_Difficulty->FindOptionIndex(Settings->GetCurrentGameDifficulty());
		ComboBox_Difficulty->SetSelectedIndex(DiffIdx != INDEX_NONE ? DiffIdx : 1 /*Normal*/);
	}
}

void UPDOptionsScreenWidget::HandleApplyClicked()
{
	UPDGameUserSettings* Settings = UPDGameUserSettings::Get();
	if (!Settings)
	{
		return;
	}

	if (ComboBox_Resolution)
	{
		const int32 Idx = ComboBox_Resolution->GetSelectedIndex();
		if (SupportedResolutions.IsValidIndex(Idx))
		{
			Settings->SetScreenResolution(SupportedResolutions[Idx]);
		}
	}

	if (ComboBox_WindowMode)
	{
		Settings->SetFullscreenMode(static_cast<EWindowMode::Type>(ComboBox_WindowMode->GetSelectedIndex()));
	}

	if (ComboBox_Quality)
	{
		Settings->SetOverallScalabilityLevel(ComboBox_Quality->GetSelectedIndex());
	}

	if (CheckBox_VSync)
	{
		Settings->SetVSyncEnabled(CheckBox_VSync->IsChecked());
	}

	if (Slider_Gamma)
	{
		Settings->SetCurrentDisplayGamma(Slider_Gamma->GetValue());
	}

	if (Slider_OverallVolume)
	{
		Settings->SetOverallVolume(Slider_OverallVolume->GetValue());
	}
	if (Slider_MusicVolume)
	{
		Settings->SetMusicVolume(Slider_MusicVolume->GetValue());
	}
	if (Slider_SoundFXVolume)
	{
		Settings->SetSoundFXVolume(Slider_SoundFXVolume->GetValue());
	}
	if (CheckBox_BackgroundAudio)
	{
		Settings->SetAllowBackgroundAudio(CheckBox_BackgroundAudio->IsChecked());
	}
	if (CheckBox_HDRAudio)
	{
		Settings->SetUseHDRAudioMode(CheckBox_HDRAudio->IsChecked());
	}

	if (ComboBox_Difficulty)
	{
		Settings->SetCurrentGameDifficulty(ComboBox_Difficulty->GetSelectedOption());
	}

	Settings->ApplyResolutionSettings(/*bCheckForCommandLineOverrides=*/false);
	Settings->ApplySettings(/*bCheckForCommandLineOverrides=*/false);
	Settings->SaveSettings();
}

void UPDOptionsScreenWidget::HandleBackClicked()
{
	if (UPDFrontendUISubsystem* Sub = UPDFrontendUISubsystem::Get(this))
	{
		Sub->PopFromLayer(EUILayer::Frontend);
	}
}