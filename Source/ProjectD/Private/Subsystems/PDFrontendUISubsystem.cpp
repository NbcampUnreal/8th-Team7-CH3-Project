// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/PDFrontendUISubsystem.h"

#include "Blueprint/UserWidget.h"
#include "Widgets/PDActivatableBase.h"
#include "Widgets/PDRootLayout.h"
#include "Widgets/PDWidgetStack.h"

namespace
{
	constexpr EUILayer GAllLayers[] = { EUILayer::Frontend, EUILayer::GameMenu, EUILayer::Modal };

}

UPDFrontendUISubsystem* UPDFrontendUISubsystem::Get(const UObject* WorldContextObject)
{
	if (GEngine)
	{
		UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::Assert);
		return UGameInstance::GetSubsystem<UPDFrontendUISubsystem>(World->GetGameInstance());
	}
	return nullptr;
}

void UPDFrontendUISubsystem::RegisterRootLayout(UPDRootLayout* InRootLayout)
{
	if (RootLayout == InRootLayout) return;


	// ê¸°ì¡´ ?±ë¡ ?´ì œ
	if (RootLayout)
	{
		for (EUILayer Layer : GAllLayers)
		{
			if (UPDWidgetStack* Stack = RootLayout->GetLayerStack(Layer))
			{
				Stack->OnStackChanged.RemoveAll(this);
			}
		}
	}

	RootLayout = InRootLayout;

	// ? ê·œ ?±ë¡ ë°??¸ë¦¬ê²Œì´??êµ¬ë…
	if (RootLayout)
	{
		for (EUILayer Layer : GAllLayers)
		{
			if (UPDWidgetStack* Stack = RootLayout->GetLayerStack(Layer))
			{
				Stack->OnStackChanged.AddUObject(this, &UPDFrontendUISubsystem::HandleAnyStackChanged);
			}
		}

		FlushPendingInitialPush();
	}

	OnEffectiveUIStateChanged.Broadcast(ComputeEffectiveInputMode());

}

void UPDFrontendUISubsystem::UnregisterRootLayout()
{
	if (!RootLayout) return;


	for (EUILayer Layer : GAllLayers)
	{
		if (UPDWidgetStack* Stack = RootLayout->GetLayerStack(Layer))
		{
			Stack->OnStackChanged.RemoveAll(this);
		}
	}
	RootLayout = nullptr;
}

UPDActivatableBase* UPDFrontendUISubsystem::PushToLayer(EUILayer Layer, TSubclassOf<UPDActivatableBase> ScreenClass)
{

	if (!RootLayout)
	{
		UE_LOG(LogTemp, Warning, TEXT("PushToLayer called but RootLayout is not registered."));
		return nullptr;
	}
	UPDWidgetStack* Stack = RootLayout->GetLayerStack(Layer);
	UPDActivatableBase* Pushed = Stack ? Stack->Push(ScreenClass) : nullptr;
	return Pushed;
}

void UPDFrontendUISubsystem::RequestInitialPush(EUILayer Layer, TSubclassOf<UPDActivatableBase> ScreenClass)
{
	if (!ScreenClass)
	{
		return;
	}

	if (RootLayout)
	{
		PushToLayer(Layer, ScreenClass);
		return;
	}

	PendingInitialPushLayer = Layer;
	PendingInitialPushClass = ScreenClass;
	bHasPendingInitialPush = true;
}

void UPDFrontendUISubsystem::FlushPendingInitialPush()
{
	if (!bHasPendingInitialPush || !PendingInitialPushClass)
	{
		bHasPendingInitialPush = false;
		PendingInitialPushClass = nullptr;
		return;
	}

	const EUILayer Layer = PendingInitialPushLayer;
	TSubclassOf<UPDActivatableBase> Class = PendingInitialPushClass;

	bHasPendingInitialPush = false;
	PendingInitialPushClass = nullptr;

	PushToLayer(Layer, Class);
}

void UPDFrontendUISubsystem::PopFromLayer(EUILayer Layer)
{
	if (!RootLayout) return;
	if (UPDWidgetStack* Stack = RootLayout->GetLayerStack(Layer))
	{
		Stack->Pop();
	}
}

void UPDFrontendUISubsystem::ClearLayer(EUILayer Layer)
{
	if (!RootLayout) return;
	if (UPDWidgetStack* Stack = RootLayout->GetLayerStack(Layer))
	{
		Stack->Clear();
	}
}

UPDActivatableBase* UPDFrontendUISubsystem::GetTopOfLayer(EUILayer Layer) const
{
	if (!RootLayout) return nullptr;
	UPDWidgetStack* Stack = RootLayout->GetLayerStack(Layer);
	return Stack ? Stack->GetTop() : nullptr;
}

void UPDFrontendUISubsystem::HandleAnyStackChanged(UPDWidgetStack* ChangedStack)
{
	const EWidgetInputMode Mode = ComputeEffectiveInputMode();
	OnEffectiveUIStateChanged.Broadcast(Mode);
}

EWidgetInputMode UPDFrontendUISubsystem::ComputeEffectiveInputMode() const
{
	if (!RootLayout) return EWidgetInputMode::Game;

	// ?°ì„ ?œìœ„(=z-order top down): Modal > GameMenu > Frontend. ëª¨ë‘ ë¹„ë©´ Game (?¸ê²Œ???íƒœ).
	static constexpr EUILayer Priority[] = { EUILayer::Modal, EUILayer::GameMenu, EUILayer::Frontend };
	for (EUILayer Layer : Priority)
	{
		if (UPDWidgetStack* Stack = RootLayout->GetLayerStack(Layer))
		{
			if (UPDActivatableBase* Top = Stack->GetTop())
			{
				return Top->GetInputMode();
			}
		}
	}
	return EWidgetInputMode::Game;
}
