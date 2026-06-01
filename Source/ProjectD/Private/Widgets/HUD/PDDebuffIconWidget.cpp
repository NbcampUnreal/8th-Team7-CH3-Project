// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/HUD/PDDebuffIconWidget.h"

#include "Components/Image.h"
#include "Materials/MaterialInterface.h"

void UPDDebuffIconWidget::SetIconData(UMaterialInterface* IconMaterial)
{
	if (!Image_Icon || !IconMaterial) return;

	Image_Icon->SetBrushFromMaterial(IconMaterial);
}
