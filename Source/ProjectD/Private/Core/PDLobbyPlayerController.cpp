// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/PDLobbyPlayerController.h"

#include "Core/PDGameInstance.h"
#include "Core/PDLobbyGameState.h"
#include "Subsystems/PDFrontendUISubsystem.h"
#include "Type/Types.h"
#include "Widgets/PDActivatableBase.h"

void APDLobbyPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	// 최초 진입은 MainMenu(LobbyScreenClass). Standalone/빌드에서 호스팅·참가로 재진입 시엔
	// GameInstance 플래그를 소비해 방 화면(RoomScreenClass)으로 직행.
	TSubclassOf<UPDActivatableBase> ScreenToPush = LobbyScreenClass;
	if (UPDGameInstance* GI = GetGameInstance<UPDGameInstance>())
	{
		if (GI->ConsumePendingRoomScreen() && RoomScreenClass)
		{
			ScreenToPush = RoomScreenClass;
		}
	}

	if (!ScreenToPush)
	{
		return;
	}

	if (UPDFrontendUISubsystem* Subsystem = UPDFrontendUISubsystem::Get(this))
	{
		Subsystem->RequestInitialPush(EUILayer::Frontend, ScreenToPush);
	}
}

void APDLobbyPlayerController::Server_SetRoomJoined_Implementation(bool bJoined)
{
	if (UWorld* World = GetWorld())
	{
		if (APDLobbyGameState* GS = World->GetGameState<APDLobbyGameState>())
		{
			GS->SetPlayerJoined(PlayerState, bJoined);
		}
	}
}