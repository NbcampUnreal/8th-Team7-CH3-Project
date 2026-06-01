// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "PDLobbyGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPDOnLobbyPlayersChanged);

/**
 * Lobby/StartupLevel용 GameState.
 * AGameStateBase::PlayerArray가 이미 replicate되므로 별도 닉네임 리스트를 두지 않는다.
 * Add/RemovePlayerState 시 LobbyVersion을 증가시켜 클라이언트의 OnRep에서
 * OnLobbyPlayersChanged 델리게이트가 브로드캐스트되도록 한다.
 */
UCLASS()
class PROJECTD_API APDLobbyGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	virtual void AddPlayerState(APlayerState* PlayerState) override;
	virtual void RemovePlayerState(APlayerState* PlayerState) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(BlueprintAssignable, Category = "PD|Lobby")
	FPDOnLobbyPlayersChanged OnLobbyPlayersChanged;

	/** 방을 연 호스트의 PlayerState. 첫 AddPlayerState로 결정. 클라이언트에도 replicate됨. */
	UFUNCTION(BlueprintPure, Category = "PD|Lobby")
	APlayerState* GetHostPlayerState() const { return HostPlayerState; }

	/** 방 화면(LobbyScreen)에 실제로 입장한 플레이어만. PlayerArray(=연결된 전원)와 구분.
	 *  PIE Listen Server는 전원이 자동 연결되므로, 방에 들어온 사람만 집계하려면 이 목록을 본다. */
	const TArray<TObjectPtr<APlayerState>>& GetJoinedPlayers() const { return JoinedPlayers; }

	/** 서버 전용. LobbyScreen 진입/이탈 시 PC의 ServerRPC를 통해 호출. */
	void SetPlayerJoined(APlayerState* PlayerState, bool bJoined);

protected:
	UPROPERTY(ReplicatedUsing = OnRep_LobbyVersion)
	int32 LobbyVersion = 0;

	UPROPERTY(Replicated)
	TObjectPtr<APlayerState> HostPlayerState;

	UPROPERTY(Replicated)
	TArray<TObjectPtr<APlayerState>> JoinedPlayers;

	UFUNCTION()
	void OnRep_LobbyVersion();
};