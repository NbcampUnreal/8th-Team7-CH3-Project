// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/PDLobbyGameMode.h"

#include "Core/PDLobbyGameState.h"
#include "GameFramework/GameSession.h"

APDLobbyGameMode::APDLobbyGameMode()
{
	GameStateClass = APDLobbyGameState::StaticClass();
}

void APDLobbyGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	if (GameSession)
	{
		GameSession->MaxPlayers = MaxLobbyPlayers;
	}
}