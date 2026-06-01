// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/Lobby/PDPlayModeSelectScreenWidget.h"

#include "Components/Button.h"
#include "Core/PDGameInstance.h"
#include "Core/PDSessionService.h"
#include "GameFramework/PlayerController.h"
#include "Subsystems/PDFrontendUISubsystem.h"
#include "Type/Types.h"

void UPDPlayModeSelectScreenWidget::NativeOnActivated()
{
	Super::NativeOnActivated();

	if (Button_Host)
	{
		Button_Host->OnClicked.AddDynamic(this, &UPDPlayModeSelectScreenWidget::HandleHostClicked);
	}
	if (Button_Join)
	{
		Button_Join->OnClicked.AddDynamic(this, &UPDPlayModeSelectScreenWidget::HandleJoinClicked);
	}
	if (Button_Back)
	{
		Button_Back->OnClicked.AddDynamic(this, &UPDPlayModeSelectScreenWidget::HandleBackClicked);
	}
}

void UPDPlayModeSelectScreenWidget::NativeOnDeactivated()
{
	if (Button_Host)
	{
		Button_Host->OnClicked.RemoveDynamic(this, &UPDPlayModeSelectScreenWidget::HandleHostClicked);
	}
	if (Button_Join)
	{
		Button_Join->OnClicked.RemoveDynamic(this, &UPDPlayModeSelectScreenWidget::HandleJoinClicked);
	}
	if (Button_Back)
	{
		Button_Back->OnClicked.RemoveDynamic(this, &UPDPlayModeSelectScreenWidget::HandleBackClicked);
	}

	Super::NativeOnDeactivated();
}

UWidget* UPDPlayModeSelectScreenWidget::GetDesiredFocusTarget_Implementation() const
{
	return Button_Host;
}

void UPDPlayModeSelectScreenWidget::HandleHostClicked()
{
	// PIE Listen Server 등 이미 연결된 상태 → 세션 작업 불필요, 방 화면으로 전환만.
	if (IsAlreadyConnected())
	{
		PushLobbyScreen();
		return;
	}

	// Standalone(빌드/Steam) → 실제 호스팅. 레벨 트래블 후 방 화면으로 직행하도록 플래그 set.
	UPDGameInstance* GI = GetGameInstance<UPDGameInstance>();
	if (!GI)
	{
		return;
	}
	UPDSessionService* Service = GI->GetSessionService();
	if (!Service)
	{
		UE_LOG(LogTemp, Warning, TEXT("UPDPlayModeSelectScreenWidget: SessionService unavailable"));
		return;
	}

	GI->SetPendingRoomScreen();
	Service->HostSession(MaxLobbyPlayers, GI->GetLobbyLevel());
}

void UPDPlayModeSelectScreenWidget::HandleJoinClicked()
{
	// 이미 연결된 상태(Client) → 방 화면으로 전환만.
	if (IsAlreadyConnected())
	{
		PushLobbyScreen();
		return;
	}

	UPDGameInstance* GI = GetGameInstance<UPDGameInstance>();
	if (!GI)
	{
		return;
	}
	UPDSessionService* Service = GI->GetSessionService();
	if (!Service)
	{
		UE_LOG(LogTemp, Warning, TEXT("UPDPlayModeSelectScreenWidget: SessionService unavailable"));
		return;
	}

	GI->SetPendingRoomScreen();
	Service->JoinSession(GetOwningPlayer());
}

void UPDPlayModeSelectScreenWidget::HandleBackClicked()
{
	if (UPDFrontendUISubsystem* Sub = UPDFrontendUISubsystem::Get(this))
	{
		Sub->PopFromLayer(EUILayer::Frontend);
	}
}

bool UPDPlayModeSelectScreenWidget::IsAlreadyConnected() const
{
	const UWorld* World = GetWorld();
	return World && World->GetNetMode() != NM_Standalone;
}

void UPDPlayModeSelectScreenWidget::PushLobbyScreen()
{
	if (!LobbyScreenClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("UPDPlayModeSelectScreenWidget: LobbyScreenClass not set"));
		return;
	}

	if (UPDFrontendUISubsystem* Sub = UPDFrontendUISubsystem::Get(this))
	{
		Sub->PushToLayer(EUILayer::Frontend, LobbyScreenClass);
	}
}