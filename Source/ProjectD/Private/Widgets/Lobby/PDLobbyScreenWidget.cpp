// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/Lobby/PDLobbyScreenWidget.h"

#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Core/PDGameInstance.h"

void UPDLobbyScreenWidget::NativeOnActivated()
{
	Super::NativeOnActivated();

	if (Button_NewGame)
	{
		Button_NewGame->OnClicked.AddDynamic(this, &UPDLobbyScreenWidget::HandleNewGameClicked);
	}
	if (Button_Continue)
	{
		Button_Continue->OnClicked.AddDynamic(this, &UPDLobbyScreenWidget::HandleContinueClicked);
		Button_Continue->SetIsEnabled(false);
	}
	if (Button_Settings)
	{
		Button_Settings->OnClicked.AddDynamic(this, &UPDLobbyScreenWidget::HandleSettingsClicked);
	}
	if (Button_Quit)
	{
		Button_Quit->OnClicked.AddDynamic(this, &UPDLobbyScreenWidget::HandleQuitClicked);
	}
	if (Button_HostGame)
	{
		Button_HostGame->OnClicked.AddDynamic(this, &UPDLobbyScreenWidget::HandleHostGameClicked);
	}
	if (Button_JoinGame)
	{
		Button_JoinGame->OnClicked.AddDynamic(this, &UPDLobbyScreenWidget::HandleJoinGameClicked);
	}
	if (TextBox_HostAddress && TextBox_HostAddress->GetText().IsEmpty())
	{
		TextBox_HostAddress->SetText(FText::FromString(DefaultHostAddress));
	}
}

void UPDLobbyScreenWidget::NativeOnDeactivated()
{
	if (Button_NewGame)
	{
		Button_NewGame->OnClicked.RemoveDynamic(this, &UPDLobbyScreenWidget::HandleNewGameClicked);
	}
	if (Button_Continue)
	{
		Button_Continue->OnClicked.RemoveDynamic(this, &UPDLobbyScreenWidget::HandleContinueClicked);
	}
	if (Button_Settings)
	{
		Button_Settings->OnClicked.RemoveDynamic(this, &UPDLobbyScreenWidget::HandleSettingsClicked);
	}
	if (Button_Quit)
	{
		Button_Quit->OnClicked.RemoveDynamic(this, &UPDLobbyScreenWidget::HandleQuitClicked);
	}
	if (Button_HostGame)
	{
		Button_HostGame->OnClicked.RemoveDynamic(this, &UPDLobbyScreenWidget::HandleHostGameClicked);
	}
	if (Button_JoinGame)
	{
		Button_JoinGame->OnClicked.RemoveDynamic(this, &UPDLobbyScreenWidget::HandleJoinGameClicked);
	}

	Super::NativeOnDeactivated();
}

UWidget* UPDLobbyScreenWidget::GetDesiredFocusTarget_Implementation() const
{
	return Button_NewGame;
}

void UPDLobbyScreenWidget::HandleNewGameClicked()
{
	if (MainLevel.IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("UPDLobbyScreenWidget::HandleNewGameClicked: MainLevel is not set."));
		return;
	}

	UPDGameInstance* GI = GetGameInstance<UPDGameInstance>();
	if (!GI) return;

	GI->TravelToLevel(MainLevel, /*bMarkBaseResetPending=*/false);
}

void UPDLobbyScreenWidget::HandleContinueClicked()
{
}

void UPDLobbyScreenWidget::HandleSettingsClicked()
{
}

void UPDLobbyScreenWidget::HandleQuitClicked()
{
}

void UPDLobbyScreenWidget::HandleHostGameClicked()
{
	if (MainLevel.IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("UPDLobbyScreenWidget::HandleHostGameClicked: MainLevel is not set."));
		return;
	}

	UPDGameInstance* GI = GetGameInstance<UPDGameInstance>();
	if (!GI) return;

	GI->HostHamachiGame(MainLevel, HamachiPort);
}

void UPDLobbyScreenWidget::HandleJoinGameClicked()
{
	FString Address = DefaultHostAddress;
	if (TextBox_HostAddress)
	{
		Address = TextBox_HostAddress->GetText().ToString();
	}

	UPDGameInstance* GI = GetGameInstance<UPDGameInstance>();
	if (!GI) return;

	GI->JoinHamachiGame(Address, HamachiPort);
}
