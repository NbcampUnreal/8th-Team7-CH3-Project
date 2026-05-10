// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/PDFrontendUISubsystem.h"

#include "Blueprint/UserWidget.h"
#include "Widgets/PDActivatableBase.h"

UPDFrontendUISubsystem* UPDFrontendUISubsystem::Get(const UObject* WorldContextObject)
{
	if (GEngine)
	{
		UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::Assert);
		return UGameInstance::GetSubsystem<UPDFrontendUISubsystem>(World->GetGameInstance());
	}
	return nullptr;
}

UPDActivatableBase* UPDFrontendUISubsystem::OpenScreen(TSubclassOf<UPDActivatableBase> ScreenClass)
{
	if (!ScreenClass) return nullptr;

	// 기존 화면 자동 정리
	if (CurrentScreen)
	{
		if (CurrentScreen->IsActivated())
		{
			CurrentScreen->Deactivate();
		}
		CurrentScreen->RemoveFromParent();
		CurrentScreen = nullptr;
	}

	UPDActivatableBase* NewScreen = CreateWidget<UPDActivatableBase>(GetGameInstance(), ScreenClass);
	if (!NewScreen) return nullptr;

	NewScreen->AddToViewport(ScreenZOrder);
	NewScreen->Activate();
	CurrentScreen = NewScreen;
	return NewScreen;
}

void UPDFrontendUISubsystem::CloseScreen()
{
	if (!CurrentScreen) return;

	if (CurrentScreen->IsActivated())
	{
		CurrentScreen->Deactivate();
	}
	CurrentScreen->RemoveFromParent();
	CurrentScreen = nullptr;
}
