// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "PDStartupGameMode.generated.h"

/**
 * StartupLevel용 GameMode.
 * - PlayerControllerClass = APDStartupPlayerController (메인 메뉴 push)
 * - GameStateClass = 기본 AGameStateBase (PlayerList replication 불필요)
 * - 멀티 연결 전 standalone 단계로만 사용. 호스트가 "방 만들기" 누르면 OpenLevel(LobbyLevel?listen)로 전환.
 */
UCLASS()
class PROJECTD_API APDStartupGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	APDStartupGameMode();
};