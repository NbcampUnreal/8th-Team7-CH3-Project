#include "Widgets/HUD/PDActionPromptItemWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

void UPDActionPromptItemWidget::SetPrompt(UTexture2D* InIcon, const FText& InActionText)
{
	if (Image_Key)
	{
		Image_Key->SetBrushFromTexture(InIcon);
		Image_Key->SetVisibility(InIcon ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}

	if (Text_Action)
	{
		Text_Action->SetText(InActionText);
	}
}