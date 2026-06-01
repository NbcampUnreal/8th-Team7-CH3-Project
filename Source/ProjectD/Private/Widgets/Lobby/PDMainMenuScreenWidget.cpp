// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/Lobby/PDMainMenuScreenWidget.h"

#include "Components/Button.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Subsystems/PDFrontendUISubsystem.h"
#include "Type/Types.h"

void UPDMainMenuScreenWidget::NativeOnActivated()
{
	Super::NativeOnActivated();

	if (Button_NewGame)
	{
		Button_NewGame->OnClicked.AddDynamic(this, &UPDMainMenuScreenWidget::HandleNewGameClicked);
	}
	if (Button_Continue)
	{
		Button_Continue->OnClicked.AddDynamic(this, &UPDMainMenuScreenWidget::HandleContinueClicked);
		Button_Continue->SetIsEnabled(false);
	}
	if (Button_Settings)
	{
		Button_Settings->OnClicked.AddDynamic(this, &UPDMainMenuScreenWidget::HandleSettingsClicked);
	}
	if (Button_Quit)
	{
		Button_Quit->OnClicked.AddDynamic(this, &UPDMainMenuScreenWidget::HandleQuitClicked);
	}
}

void UPDMainMenuScreenWidget::NativeOnDeactivated()
{
	if (Button_NewGame)
	{
		Button_NewGame->OnClicked.RemoveDynamic(this, &UPDMainMenuScreenWidget::HandleNewGameClicked);
	}
	if (Button_Continue)
	{
		Button_Continue->OnClicked.RemoveDynamic(this, &UPDMainMenuScreenWidget::HandleContinueClicked);
	}
	if (Button_Settings)
	{
		Button_Settings->OnClicked.RemoveDynamic(this, &UPDMainMenuScreenWidget::HandleSettingsClicked);
	}
	if (Button_Quit)
	{
		Button_Quit->OnClicked.RemoveDynamic(this, &UPDMainMenuScreenWidget::HandleQuitClicked);
	}

	Super::NativeOnDeactivated();
}

UWidget* UPDMainMenuScreenWidget::GetDesiredFocusTarget_Implementation() const
{
	return Button_NewGame;
}

void UPDMainMenuScreenWidget::HandleNewGameClicked()
{
	if (!PlayModeSelectScreenClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("UPDMainMenuScreenWidget: PlayModeSelectScreenClass not set"));
		return;
	}

	if (UPDFrontendUISubsystem* Sub = UPDFrontendUISubsystem::Get(this))
	{
		Sub->PushToLayer(EUILayer::Frontend, PlayModeSelectScreenClass);
	}
}

void UPDMainMenuScreenWidget::HandleContinueClicked()
{
}

void UPDMainMenuScreenWidget::HandleSettingsClicked()
{
	if (!OptionsScreenClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("UPDMainMenuScreenWidget: OptionsScreenClass not set"));
		return;
	}

	if (UPDFrontendUISubsystem* Sub = UPDFrontendUISubsystem::Get(this))
	{
		Sub->PushToLayer(EUILayer::Frontend, OptionsScreenClass);
	}
}

void UPDMainMenuScreenWidget::HandleQuitClicked()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}