// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/Lobby/PDLobbyScreenWidget.h"

#include "Components/Button.h"
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