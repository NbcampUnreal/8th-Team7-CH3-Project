// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/PDGameUserSettings.h"

#include "AudioDevice.h"
#include "Engine/Engine.h"

UPDGameUserSettings::UPDGameUserSettings()
	: CurrentGameDifficulty(TEXT("Normal"))
	, OverallVolume(1.f)
	, MusicVolume(1.f)
	, SoundFXVolume(1.f)
	, bAllowBackgroundAudio(false)
	, bUseHDRAudioMode(false)
{
}

UPDGameUserSettings* UPDGameUserSettings::Get()
{
	return GEngine ? Cast<UPDGameUserSettings>(GEngine->GetGameUserSettings()) : nullptr;
}

void UPDGameUserSettings::SetOverallVolume(float InVolume)
{
	OverallVolume = InVolume;
	ApplyAudioSettings();
}

void UPDGameUserSettings::ApplyAudioSettings()
{
	if (!GEngine)
	{
		return;
	}

	// 전체 볼륨 = 메인 오디오 디바이스의 transient primary volume.
	// SoundClass 없이 모든 소리에 곱연산으로 적용된다.
	if (FAudioDeviceHandle Device = GEngine->GetMainAudioDevice())
	{
		Device->SetTransientPrimaryVolume(OverallVolume);
	}
}

void UPDGameUserSettings::SetMusicVolume(float InVolume)
{
	MusicVolume = InVolume;
	// TODO: Music SoundClass 연동.
}

void UPDGameUserSettings::SetSoundFXVolume(float InVolume)
{
	SoundFXVolume = InVolume;
	// TODO: SFX SoundClass 연동.
}

float UPDGameUserSettings::GetCurrentDisplayGamma() const
{
	return GEngine ? GEngine->GetDisplayGamma() : 0.f;
}

void UPDGameUserSettings::SetCurrentDisplayGamma(float InGamma)
{
	if (GEngine)
	{
		GEngine->DisplayGamma = InGamma;
	}
}