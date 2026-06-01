// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/PDActivatableBase.h"

#include "Core/PDPlayerController.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTag/PDGameplayTags.h"
#include "Subsystems/PDFrontendUISubsystem.h"
#include "Type/Types.h"

void UPDActivatableBase::Activate()
{
	if (bActivated) return;
	bActivated = true;

	ApplyInputMode();
	NativeOnActivated();
	OnActivated();
}

void UPDActivatableBase::Deactivate()
{
	if (!bActivated) return;
	bActivated = false;

	NativeOnDeactivated();
	OnDeactivated();
}

UWidget* UPDActivatableBase::GetDesiredFocusTarget_Implementation() const
{
	return nullptr;
}

void UPDActivatableBase::NativeOnActivated()
{
	// UIOnly(Menu) 화면은 ESC 등 키 입력을 직접 받아야 하므로 활성 시 키보드 포커스를 가져온다.
	// GameAndMenu/Game은 게임 입력 유지를 위해 포커스를 강제하지 않는다.
	if (InputMode == EWidgetInputMode::Menu || InputMode == EWidgetInputMode::Cinematic)
	{
		SetIsFocusable(true);
		if (APlayerController* PC = GetOwningPlayer())
		{
			SetUserFocus(PC);
		}
	}
}

void UPDActivatableBase::NativeOnDeactivated()
{
}

void UPDActivatableBase::NativeOnFocusLost(const FFocusEvent& InFocusEvent)
{
	Super::OnFocusLost(InFocusEvent);

	if (!bLightDismissable) return;

	UPDFrontendUISubsystem* Subsystem = GetGameInstance()->GetSubsystem<UPDFrontendUISubsystem>();
	if (!Subsystem) return;

	// 자기 자신이 top인 레이어를 찾아서 pop. 어느 레이어에도 top이 아니면 무시
	static constexpr EUILayer AllLayers[] = { EUILayer::Frontend, EUILayer::GameMenu, EUILayer::Modal };
	for (EUILayer Layer : AllLayers)
	{
		if (Subsystem->GetTopOfLayer(Layer) == this)
		{
			Subsystem->PopFromLayer(Layer);
			return;
		}
	}
}

FReply UPDActivatableBase::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (IsUIBackKey(InKeyEvent.GetKey()))
	{
		if (HandleEscape())
		{
			return FReply::Handled();
		}
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

bool UPDActivatableBase::HandleEscape_Implementation()
{
	UPDFrontendUISubsystem* Subsystem = UPDFrontendUISubsystem::Get(this);
	if (!Subsystem)
	{
		return false;
	}

	// 우선순위(z-order top down)로 this가 top인 레이어를 찾는다.
	static constexpr EUILayer LayerSearchOrder[] = { EUILayer::Modal, EUILayer::GameMenu, EUILayer::Frontend };
	for (EUILayer Layer : LayerSearchOrder)
	{
		if (Subsystem->GetTopOfLayer(Layer) == this)
		{
			// top이면 ESC를 소비한다(에디터 Stop PIE로 새지 않도록). pop은 허용 시에만.
			if (bAllowEscapeDismiss)
			{
				Subsystem->PopFromLayer(Layer);
			}
			return true;
		}
	}
	return false;
}

bool UPDActivatableBase::IsUIBackKey(const FKey& InKey) const
{
	// Input_UIBack에 매핑된 키를 동적 조회(IMC 주도). 매핑이 없으면 Escape 폴백.
	// UIOnly 모드에선 Enhanced Input이 차단되므로 InputComponent 바인딩이 아니라 키 목록 조회로만 사용한다.
	if (const APDPlayerController* PC = Cast<APDPlayerController>(GetOwningPlayer()))
	{
		const TArray<FKey> BackKeys = PC->GetMappedKeysForInputTag(PDGameplayTags::Input_UIBack);
		if (BackKeys.Num() > 0)
		{
			return BackKeys.Contains(InKey);
		}
	}
	return InKey == EKeys::Escape;
}

void UPDActivatableBase::ApplyInputMode()
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC) return;

	switch (InputMode)
	{
	case EWidgetInputMode::Game:
		{
			FInputModeGameOnly Mode;
			PC->SetInputMode(Mode);
			PC->SetShowMouseCursor(false);
			break;
		}
	case EWidgetInputMode::GameAndMenu:
		{
			FInputModeGameAndUI Mode;
			Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			Mode.SetHideCursorDuringCapture(false);
			PC->SetInputMode(Mode);
			PC->SetShowMouseCursor(bShowMouseCursor);
			break;
		}
	case EWidgetInputMode::Menu:
		{
			FInputModeUIOnly Mode;
			Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PC->SetInputMode(Mode);
			PC->SetShowMouseCursor(bShowMouseCursor);
			break;
		}
	case EWidgetInputMode::Cinematic:
		{
			FInputModeUIOnly Mode;
			Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PC->SetInputMode(Mode);
			PC->SetShowMouseCursor(false);
			break;
		}
	case EWidgetInputMode::Passive:
		{
			break;
		}
	}
}
