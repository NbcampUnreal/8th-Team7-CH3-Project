// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "PDLobbyGameMode.generated.h"

/**
 * StartupLevel / LobbyLevel용 GameMode.
 * Lobby 화면 push는 PlayerControllerClass(=APDLobbyPlayerController)가 자체적으로 처리한다.
 * 본 클래스는 GameStateClass(=APDLobbyGameState) 지정 + MaxPlayers 제한 등 lobby 전용 server-side 설정 지점.
 */
UCLASS()
class PROJECTD_API APDLobbyGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	APDLobbyGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "PD|Lobby", meta = (ClampMin = "1"))
	int32 MaxLobbyPlayers = 4;
};