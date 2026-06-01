// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "PDGameUserSettings.generated.h"

/**
 * ProjectD 전용 GameUserSettings.
 * Video(해상도/화면모드/품질/VSync)는 UGameUserSettings 기본 API를 그대로 사용한다.
 * Audio/Gameplay 등 커스텀 설정은 향후 이 클래스에 UPROPERTY(Config) + getter/setter로 확장한다.
 * (참조: FrontendUI의 UFrontendGameUserSettings)
 *
 * GEngine->GetGameUserSettings()가 이 타입을 반환하려면
 * DefaultEngine.ini [/Script/Engine.Engine] GameUserSettingsClassName 지정이 필요하다.
 */
UCLASS()
class PROJECTD_API UPDGameUserSettings : public UGameUserSettings
{
	GENERATED_BODY()

public:
	UPDGameUserSettings();

	UFUNCTION(BlueprintPure, Category = "PD|Settings")
	static UPDGameUserSettings* Get();

	// ───── Gameplay ─────
	UFUNCTION(BlueprintPure, Category = "PD|Settings|Gameplay")
	FString GetCurrentGameDifficulty() const { return CurrentGameDifficulty; }

	UFUNCTION(BlueprintCallable, Category = "PD|Settings|Gameplay")
	void SetCurrentGameDifficulty(const FString& InDifficulty) { CurrentGameDifficulty = InDifficulty; }

	// ───── Audio (현재는 값 저장만 — 실제 적용은 ProjectD SoundClass/SoundMix 연동 시) ─────
	UFUNCTION(BlueprintPure, Category = "PD|Settings|Audio")
	float GetOverallVolume() const { return OverallVolume; }

	UFUNCTION(BlueprintCallable, Category = "PD|Settings|Audio")
	void SetOverallVolume(float InVolume);

	UFUNCTION(BlueprintPure, Category = "PD|Settings|Audio")
	float GetMusicVolume() const { return MusicVolume; }

	UFUNCTION(BlueprintCallable, Category = "PD|Settings|Audio")
	void SetMusicVolume(float InVolume);

	UFUNCTION(BlueprintPure, Category = "PD|Settings|Audio")
	float GetSoundFXVolume() const { return SoundFXVolume; }

	UFUNCTION(BlueprintCallable, Category = "PD|Settings|Audio")
	void SetSoundFXVolume(float InVolume);

	UFUNCTION(BlueprintPure, Category = "PD|Settings|Audio")
	bool GetAllowBackgroundAudio() const { return bAllowBackgroundAudio; }

	UFUNCTION(BlueprintCallable, Category = "PD|Settings|Audio")
	void SetAllowBackgroundAudio(bool bInAllowed) { bAllowBackgroundAudio = bInAllowed; }

	UFUNCTION(BlueprintPure, Category = "PD|Settings|Audio")
	bool GetUseHDRAudioMode() const { return bUseHDRAudioMode; }

	UFUNCTION(BlueprintCallable, Category = "PD|Settings|Audio")
	void SetUseHDRAudioMode(bool bInUseHDR) { bUseHDRAudioMode = bInUseHDR; }

	// ───── Video (감마는 GEngine 직접) ─────
	UFUNCTION(BlueprintPure, Category = "PD|Settings|Video")
	float GetCurrentDisplayGamma() const;

	UFUNCTION(BlueprintCallable, Category = "PD|Settings|Video")
	void SetCurrentDisplayGamma(float InGamma);

	/** 저장된 OverallVolume을 메인 오디오 디바이스에 적용(transient primary volume).
	 *  게임 시작 시 1회 + SetOverallVolume 시 호출되어 실제 전체 볼륨에 반영된다.
	 *  Music/SFX는 별도 SoundClass/SoundMix 인프라가 필요해 아직 미적용(값 저장만). */
	void ApplyAudioSettings();

private:
	UPROPERTY(Config)
	FString CurrentGameDifficulty;

	UPROPERTY(Config)
	float OverallVolume;

	UPROPERTY(Config)
	float MusicVolume;

	UPROPERTY(Config)
	float SoundFXVolume;

	UPROPERTY(Config)
	bool bAllowBackgroundAudio;

	UPROPERTY(Config)
	bool bUseHDRAudioMode;
};