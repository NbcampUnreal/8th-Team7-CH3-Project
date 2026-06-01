#include "Widgets/HUD/PDActionPromptListWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/VerticalBox.h"
#include "Data/PDActionPromptDataAsset.h"
#include "Data/PDKeyIconDataAsset.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Widgets/HUD/PDActionPromptItemWidget.h"

void UPDActionPromptListWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ULocalPlayer* LP = GetOwningLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Sub = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			Sub->ControlMappingsRebuiltDelegate.AddUniqueDynamic(this, &UPDActionPromptListWidget::HandleControlMappingsRebuilt);
		}
	}

	RebuildPrompts();
}

void UPDActionPromptListWidget::NativeDestruct()
{
	if (ULocalPlayer* LP = GetOwningLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Sub = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			Sub->ControlMappingsRebuiltDelegate.RemoveDynamic(this, &UPDActionPromptListWidget::HandleControlMappingsRebuilt);
		}
	}

	Super::NativeDestruct();
}

void UPDActionPromptListWidget::RebuildPrompts()
{
	if (!Container_Prompts)
	{
		return;
	}

	Container_Prompts->ClearChildren();

	UPDActionPromptDataAsset* DA = PromptDataAsset.LoadSynchronous();
	if (!DA || !ItemWidgetClass)
	{
		return;
	}

	UPDKeyIconDataAsset* IconMap = KeyIconMap.LoadSynchronous();

	for (const FPDActionPromptEntry& Entry : DA->Entries)
	{
		if (!Entry.InputAction)
		{
			continue;
		}

		const FKey Key = FindKeyForAction(Entry.InputAction);
		UTexture2D* Icon = (IconMap && Key.IsValid()) ? IconMap->ResolveIcon(Key) : nullptr;

		UPDActionPromptItemWidget* Item = CreateWidget<UPDActionPromptItemWidget>(GetOwningPlayer(), ItemWidgetClass);
		if (!Item)
		{
			continue;
		}

		Item->SetPrompt(Icon, Entry.DisplayName);
		Container_Prompts->AddChildToVerticalBox(Item);
	}
}

void UPDActionPromptListWidget::HandleControlMappingsRebuilt()
{
	RebuildPrompts();
}

FKey UPDActionPromptListWidget::FindKeyForAction(const UInputAction* Action) const
{
	if (!Action || !InputMappingContext)
	{
		return FKey();
	}

	for (const FEnhancedActionKeyMapping& Mapping : InputMappingContext->GetMappings())
	{
		if (Mapping.Action == Action)
		{
			return Mapping.Key;
		}
	}

	return FKey();
}