// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/PDWidgetStack.h"

#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Widgets/PDActivatableBase.h"

UPDActivatableBase* UPDWidgetStack::Push(TSubclassOf<UPDActivatableBase> ScreenClass)
{
	if (!ScreenClass || !ContentRoot) return nullptr;

	if (UPDActivatableBase* PrevTop = GetTop())
	{
		if (PrevTop->IsActivated())
		{
			PrevTop->Deactivate();
		}
		PrevTop->SetVisibility(ESlateVisibility::Collapsed);
	}

	UPDActivatableBase* NewScreen = CreateWidget<UPDActivatableBase>(GetOwningPlayer(), ScreenClass);
	if (!NewScreen) return nullptr;

	if (UOverlaySlot* OverlaySlot = ContentRoot->AddChildToOverlay(NewScreen))
	{
		OverlaySlot->SetHorizontalAlignment(HAlign_Fill);
		OverlaySlot->SetVerticalAlignment(VAlign_Fill);
	}
	NewScreen->SetVisibility(ESlateVisibility::Visible);
	NewScreen->Activate();
	Stack.Add(NewScreen);

	OnStackChanged.Broadcast(this);
	return NewScreen;
}

void UPDWidgetStack::Pop()
{
	if (Stack.Num() == 0) return;

	UPDActivatableBase* Top = Stack.Pop();
	if (Top)
	{
		if (Top->IsActivated())
		{
			Top->Deactivate();
		}
		Top->RemoveFromParent();
	}

	if (UPDActivatableBase* NewTop = GetTop())
	{
		NewTop->SetVisibility(ESlateVisibility::Visible);
		NewTop->Activate();
	}

	OnStackChanged.Broadcast(this);
}

void UPDWidgetStack::Clear()
{
	if (Stack.Num() == 0) return;

	for (int32 i = Stack.Num() - 1; i >= 0; --i)
	{
		if (UPDActivatableBase* W = Stack[i])
		{
			if (W->IsActivated())
			{
				W->Deactivate();
			}
			W->RemoveFromParent();
		}
	}
	Stack.Reset();

	OnStackChanged.Broadcast(this);
}

UPDActivatableBase* UPDWidgetStack::GetTop() const
{
	return Stack.Num() > 0 ? Stack.Last() : nullptr;
}
