// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/HUD/PDDebuffIconBarWidget.h"

#include "Components/PanelWidget.h"
#include "Widgets/HUD/PDDebuffIconWidget.h"

void UPDDebuffIconBarWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	if (Container_Icons)
	{
		Container_Icons->ClearChildren();
	}
	ActiveIcons.Reset();
}

void UPDDebuffIconBarWidget::SetDebuffActive(const FGameplayTag& DebuffTag, bool bActive)
{
	if (!Container_Icons) return;

	const FPDDebuffIconData* Data = TagToIconData.Find(DebuffTag);
	if (!Data) return;

	const bool bCurrentlyActive = ActiveIcons.Contains(DebuffTag);

	if (bActive && !bCurrentlyActive)
	{
		if (!IconWidgetClass) return;

		UPDDebuffIconWidget* NewIcon = CreateWidget<UPDDebuffIconWidget>(this, IconWidgetClass);
		if (!NewIcon) return;

		NewIcon->SetIconData(Data->IconMaterial);
		
		Container_Icons->AddChild(NewIcon);
		ActiveIcons.Add(DebuffTag, NewIcon);
	}
	else if (!bActive && bCurrentlyActive)
	{
		TObjectPtr<UPDDebuffIconWidget> Existing = ActiveIcons.FindRef(DebuffTag);
		if (Existing)
		{
			Container_Icons->RemoveChild(Existing);
		}
		ActiveIcons.Remove(DebuffTag);
	}
}
