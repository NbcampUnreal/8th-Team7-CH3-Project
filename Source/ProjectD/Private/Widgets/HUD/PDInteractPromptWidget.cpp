#include "Widgets/HUD/PDInteractPromptWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Data/PDKeyIconDataAsset.h"
#include "Engine/Texture2D.h"
#include "InputAction.h"
#include "InputMappingContext.h"

void UPDInteractPromptWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetVisibility(ESlateVisibility::Collapsed);
	RefreshKeyIcon();
}

void UPDInteractPromptWidget::Show(const FText& InText)
{
	if (Text_Action)
	{
		Text_Action->SetText(InText);
	}

	RefreshKeyIcon();

	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UPDInteractPromptWidget::Hide()
{
	SetVisibility(ESlateVisibility::Collapsed);
}

void UPDInteractPromptWidget::RefreshKeyIcon()
{
	if (!Image_Key)
	{
		return;
	}

	FKey FoundKey;
	if (InteractAction && InputMappingContext)
	{
		for (const FEnhancedActionKeyMapping& Mapping : InputMappingContext->GetMappings())
		{
			if (Mapping.Action == InteractAction)
			{
				FoundKey = Mapping.Key;
				break;
			}
		}
	}

	UPDKeyIconDataAsset* IconMap = KeyIconMap.LoadSynchronous();
	UTexture2D* Icon = (IconMap && FoundKey.IsValid()) ? IconMap->ResolveIcon(FoundKey) : nullptr;

	Image_Key->SetBrushFromTexture(Icon);
	Image_Key->SetVisibility(Icon ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
}