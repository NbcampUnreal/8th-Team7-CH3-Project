// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "PDLoadingScreenSettings.generated.h"

class UUserWidget;
class UTexture2D;


UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Loading Screen"))
class PROJECTD_API UPDLoadingScreenSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(Config, EditAnywhere, Category = "Loading Screen", meta = (MetaClass = "/Script/UMG.UserWidget"))
	TSoftClassPtr<UUserWidget> SoftLoadingScreenWidgetClass;

	UPROPERTY(Config, EditAnywhere, Category = "Loading Screen")
	float HoldLoadingScreenExtraSeconds = 3.f;

	UPROPERTY(Config, EditAnywhere, Category = "Loading Screen")
	bool bShouldShowLoadingScreenInEditor = false;

	/** true(디폴트): 모든 OpenLevel/Travel에 자동 표시.
	 *  false: ArmForNextTransition()을 호출한 트래블 1회만 표시.
	 *  Lobby/Hub 같은 가벼운 트래블에서 로딩화면이 거슬리면 false로 두고
	 *  실제 무거운 트래블(Lobby→Base, Base→Raid 등)에만 명시적으로 arm. */
	UPROPERTY(Config, EditAnywhere, Category = "Loading Screen")
	bool bAutoShowOnLevelLoad = true;

	UPROPERTY(Config, EditAnywhere, Category = "Loading Screen|Splash")
	TArray<TSoftObjectPtr<UTexture2D>> SplashImagePool;

	UPROPERTY(Config, EditAnywhere, Category = "Loading Screen|Splash")
	TSoftObjectPtr<UTexture2D> StaticSplashOverride;
};