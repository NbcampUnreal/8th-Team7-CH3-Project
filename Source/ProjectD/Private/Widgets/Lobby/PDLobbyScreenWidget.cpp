// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/Lobby/PDLobbyScreenWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Core/PDGameInstance.h"
#include "Core/PDLobbyGameState.h"
#include "Core/PDLobbyPlayerController.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Subsystems/PDFrontendUISubsystem.h"
#include "Type/Types.h"
#include "Widgets/Lobby/PDLobbyPlayerEntryWidget.h"

void UPDLobbyScreenWidget::NativeOnActivated()
{
	Super::NativeOnActivated();

	if (Button_StartGame)
	{
		Button_StartGame->OnClicked.AddDynamic(this, &UPDLobbyScreenWidget::HandleStartGameClicked);
	}
	if (Button_Leave)
	{
		Button_Leave->OnClicked.AddDynamic(this, &UPDLobbyScreenWidget::HandleLeaveClicked);
	}

	ApplyHostState();

	AttemptBindGameState();

	// 방 화면에 진입 → 서버에 입장 등록 (PlayerArray=연결된 전원과 구분되는 "입장" 상태).
	if (APDLobbyPlayerController* LPC = Cast<APDLobbyPlayerController>(GetOwningPlayer()))
	{
		LPC->Server_SetRoomJoined(true);
	}
}

void UPDLobbyScreenWidget::AttemptBindGameState()
{
	if (BoundGameState.IsValid())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// 클라이언트에서 GameState가 늦게 도착하거나(=null) 막 도착해 타입 매칭이 될 때까지 재시도.
	if (APDLobbyGameState* LobbyGS = World->GetGameState<APDLobbyGameState>())
	{
		BindToLobbyGameState(LobbyGS);
		return;
	}

	// GameStateSetEvent(아직 set 전) + 폴링(이미 set됐으나 타입/타이밍 어긋남) 양쪽 대비.
	if (!GameStateSetHandle.IsValid())
	{
		GameStateSetHandle = World->GameStateSetEvent.AddUObject(this, &UPDLobbyScreenWidget::HandleGameStateSet);
	}
	World->GetTimerManager().SetTimer(
		BindRetryHandle, this, &UPDLobbyScreenWidget::AttemptBindGameState, 0.25f, false);
}

void UPDLobbyScreenWidget::NativeOnDeactivated()
{
	// 방 화면 이탈 → 서버에 입장 해제.
	if (APDLobbyPlayerController* LPC = Cast<APDLobbyPlayerController>(GetOwningPlayer()))
	{
		LPC->Server_SetRoomJoined(false);
	}

	if (Button_StartGame)
	{
		Button_StartGame->OnClicked.RemoveDynamic(this, &UPDLobbyScreenWidget::HandleStartGameClicked);
	}
	if (Button_Leave)
	{
		Button_Leave->OnClicked.RemoveDynamic(this, &UPDLobbyScreenWidget::HandleLeaveClicked);
	}

	if (APDLobbyGameState* GS = BoundGameState.Get())
	{
		GS->OnLobbyPlayersChanged.RemoveDynamic(this, &UPDLobbyScreenWidget::HandleLobbyPlayersChanged);
	}
	BoundGameState.Reset();

	if (UWorld* World = GetWorld())
	{
		if (GameStateSetHandle.IsValid())
		{
			World->GameStateSetEvent.Remove(GameStateSetHandle);
			GameStateSetHandle.Reset();
		}
		World->GetTimerManager().ClearTimer(BindRetryHandle);
	}

	Super::NativeOnDeactivated();
}

UWidget* UPDLobbyScreenWidget::GetDesiredFocusTarget_Implementation() const
{
	return Button_StartGame;
}

void UPDLobbyScreenWidget::HandleStartGameClicked()
{
	if (!IsLocalPlayerHost())
	{
		return;
	}

	UPDGameInstance* GI = GetGameInstance<UPDGameInstance>();
	if (!GI)
	{
		return;
	}

	const TSoftObjectPtr<UWorld> BaseLevel = GI->GetBaseLevel();
	if (BaseLevel.IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("UPDLobbyScreenWidget::HandleStartGameClicked: BaseLevel is not set on GameInstance."));
		return;
	}

	GI->TravelToLevel(BaseLevel, /*bMarkBaseResetPending=*/false);
}

void UPDLobbyScreenWidget::HandleLeaveClicked()
{
	// 방에서 나가 PlayModeSelect로 복귀(레이어 pop).
	if (UPDFrontendUISubsystem* Sub = UPDFrontendUISubsystem::Get(this))
	{
		Sub->PopFromLayer(EUILayer::Frontend);
	}
}

void UPDLobbyScreenWidget::HandleGameStateSet(AGameStateBase* NewGameState)
{
	if (UWorld* World = GetWorld())
	{
		if (GameStateSetHandle.IsValid())
		{
			World->GameStateSetEvent.Remove(GameStateSetHandle);
			GameStateSetHandle.Reset();
		}
	}
	BindToLobbyGameState(Cast<APDLobbyGameState>(NewGameState));
}

void UPDLobbyScreenWidget::BindToLobbyGameState(APDLobbyGameState* LobbyGS)
{
	if (!LobbyGS)
	{
		return;
	}

	BoundGameState = LobbyGS;
	LobbyGS->OnLobbyPlayersChanged.AddDynamic(this, &UPDLobbyScreenWidget::HandleLobbyPlayersChanged);

	// 바인딩 성공 → 재시도 타이머/GameStateSet 폴백 정리.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BindRetryHandle);
		if (GameStateSetHandle.IsValid())
		{
			World->GameStateSetEvent.Remove(GameStateSetHandle);
			GameStateSetHandle.Reset();
		}
	}

	RefreshPlayerList();
}

void UPDLobbyScreenWidget::HandleLobbyPlayersChanged()
{
	RefreshPlayerList();
}

void UPDLobbyScreenWidget::RefreshPlayerList()
{
	if (!VerticalBox_PlayerList || !PlayerEntryClass)
	{
		return;
	}

	APDLobbyGameState* GS = BoundGameState.Get();
	if (!GS)
	{
		return;
	}

	VerticalBox_PlayerList->ClearChildren();

	// 연결된 전원(PlayerArray)이 아니라 방에 "입장한" 플레이어만 표시.
	TArray<APlayerState*> ValidPlayers;
	for (const TObjectPtr<APlayerState>& PS : GS->GetJoinedPlayers())
	{
		if (PS && !PS->IsInactive())
		{
			ValidPlayers.Add(PS);
		}
	}

	APlayerState* HostPS = GS->GetHostPlayerState();

	// 고정 MaxPlayersDisplay 슬롯: 앞쪽은 참가자, 나머지는 Empty.
	for (int32 SlotIndex = 0; SlotIndex < MaxPlayersDisplay; ++SlotIndex)
	{
		UPDLobbyPlayerEntryWidget* Entry = CreateWidget<UPDLobbyPlayerEntryWidget>(this, PlayerEntryClass);
		if (!Entry)
		{
			continue;
		}

		if (SlotIndex < ValidPlayers.Num())
		{
			APlayerState* PS = ValidPlayers[SlotIndex];
			Entry->SetPlayerState(PS, /*bIsHost=*/PS == HostPS);
		}
		else
		{
			Entry->SetEmpty();
		}

		VerticalBox_PlayerList->AddChild(Entry);
	}

	if (TextBlock_PlayerCount)
	{
		TextBlock_PlayerCount->SetText(
			FText::FromString(FString::Printf(TEXT("%d/%d"), ValidPlayers.Num(), MaxPlayersDisplay)));
	}
}

void UPDLobbyScreenWidget::ApplyHostState()
{
	const bool bHost = IsLocalPlayerHost();

	if (Button_StartGame)
	{
		Button_StartGame->SetIsEnabled(bHost);
	}
	if (TextBlock_WaitingHost)
	{
		TextBlock_WaitingHost->SetVisibility(
			bHost ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
}

bool UPDLobbyScreenWidget::IsLocalPlayerHost() const
{
	if (const APlayerController* PC = GetOwningPlayer())
	{
		return PC->HasAuthority();
	}
	return false;
}