#include "Widgets/HUD/PDSkillSlotWidget.h"

#include "Components/Image.h"
#include "Engine/Texture2D.h"

void UPDSkillSlotWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	SetSelected(false);
}

void UPDSkillSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetSelected(false);
}

void UPDSkillSlotWidget::SetSelected(bool bNewSelected)
{
	bSelected = bNewSelected;

	if (Image_SlotBG)
	{
		const TSoftObjectPtr<UTexture2D>& TargetTex = bSelected ? SelectedTexture : UnselectedTexture;
		if (UTexture2D* Tex = TargetTex.LoadSynchronous())
		{
			Image_SlotBG->SetBrushFromTexture(Tex);
		}
	}
}

void UPDSkillSlotWidget::SetSkillIcon(UTexture2D* InIcon)
{
	if (Image_SkillIcon)
	{
		Image_SkillIcon->SetBrushFromTexture(InIcon);
		Image_SkillIcon->SetVisibility(InIcon ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
}

void UPDSkillSlotWidget::SetSlotTextures(TSoftObjectPtr<UTexture2D> InSelectedTex, TSoftObjectPtr<UTexture2D> InUnselectedTex)
{
	SelectedTexture = InSelectedTex;
	UnselectedTexture = InUnselectedTex;

	if (Image_SlotBG)
	{
		const TSoftObjectPtr<UTexture2D>& TargetTex = bSelected ? SelectedTexture : UnselectedTexture;
		if (UTexture2D* Tex = TargetTex.LoadSynchronous())
		{
			Image_SlotBG->SetBrushFromTexture(Tex);
		}
	}
}