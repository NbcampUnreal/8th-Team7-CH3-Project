// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/PDLobbyGameState.h"

#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"

void APDLobbyGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(APDLobbyGameState, LobbyVersion);
	DOREPLIFETIME(APDLobbyGameState, HostPlayerState);
	DOREPLIFETIME(APDLobbyGameState, JoinedPlayers);
}

void APDLobbyGameState::AddPlayerState(APlayerState* PlayerState)
{
	Super::AddPlayerState(PlayerState);

	if (HasAuthority())
	{
		// 첫 유효 참가자 = 방을 연 호스트 (listen server에서 호스트 PC가 가장 먼저 PostLogin).
		if (!HostPlayerState && PlayerState && !PlayerState->IsInactive())
		{
			HostPlayerState = PlayerState;
		}

		++LobbyVersion;
		OnLobbyPlayersChanged.Broadcast();
	}
}

void APDLobbyGameState::RemovePlayerState(APlayerState* PlayerState)
{
	Super::RemovePlayerState(PlayerState);

	if (HasAuthority())
	{
		// 연결이 끊긴 플레이어는 입장 목록에서도 제거.
		JoinedPlayers.Remove(PlayerState);

		++LobbyVersion;
		OnLobbyPlayersChanged.Broadcast();
	}
}

void APDLobbyGameState::SetPlayerJoined(APlayerState* PlayerState, bool bJoined)
{
	if (!HasAuthority() || !PlayerState)
	{
		return;
	}

	bool bChanged = false;
	if (bJoined)
	{
		if (!JoinedPlayers.Contains(PlayerState))
		{
			JoinedPlayers.Add(PlayerState);
			bChanged = true;
		}
	}
	else
	{
		bChanged = JoinedPlayers.Remove(PlayerState) > 0;
	}

	if (bChanged)
	{
		++LobbyVersion;
		OnLobbyPlayersChanged.Broadcast();
	}
}

void APDLobbyGameState::OnRep_LobbyVersion()
{
	OnLobbyPlayersChanged.Broadcast();
}