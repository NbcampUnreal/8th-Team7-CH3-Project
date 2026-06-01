// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/PDActivatableBase.h"
#include "PDLobbyScreenWidget.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;
class UPDLobbyPlayerEntryWidget;
class APDLobbyGameState;
class AGameStateBase;


/**
 * 방(Room) 화면. 흐름상 MainMenu → PlayModeSelect → LobbyScreen 의 마지막 단계.
 * 모인 참가자 리스트 + 호스트의 게임 시작 트리거 담당.
 *   - StartGame: 호스트만. BaseLevel로 ServerTravel(전원 동반).
 *   - Leave: PlayModeSelect로 돌아가기(레이어 pop).
 *   - 클라: StartGame 비활성 + "Waiting for host...".
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class PROJECTD_API UPDLobbyScreenWidget : public UPDActivatableBase
{
	GENERATED_BODY()

protected:
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	virtual UWidget* GetDesiredFocusTarget_Implementation() const override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> Button_StartGame;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Leave;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UVerticalBox> VerticalBox_PlayerList;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TextBlock_PlayerCount;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TextBlock_WaitingHost;

	UPROPERTY(EditDefaultsOnly, Category = "PD|Lobby")
	TSubclassOf<UPDLobbyPlayerEntryWidget> PlayerEntryClass;

	UPROPERTY(EditDefaultsOnly, Category = "PD|Lobby", meta = (ClampMin = "1"))
	int32 MaxPlayersDisplay = 4;

private:
	UFUNCTION() void HandleStartGameClicked();
	UFUNCTION() void HandleLeaveClicked();
	UFUNCTION() void HandleLobbyPlayersChanged();

	void HandleGameStateSet(AGameStateBase* NewGameState);
	void AttemptBindGameState();
	void BindToLobbyGameState(APDLobbyGameState* LobbyGS);
	void RefreshPlayerList();
	void ApplyHostState();
	bool IsLocalPlayerHost() const;

	TWeakObjectPtr<APDLobbyGameState> BoundGameState;
	FDelegateHandle GameStateSetHandle;
	FTimerHandle BindRetryHandle;
};