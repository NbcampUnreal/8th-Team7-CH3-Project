// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/HUD/PDBodyPartImageWidget.h"

#include "Components/Image.h"
#include "Materials/MaterialInstanceDynamic.h"

void UPDBodyPartImageWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Image_Part)
	{
		if (PartMaterial)
		{
			// 인스턴스마다 다른 MI 적용
			Image_Part->SetBrushFromMaterial(PartMaterial);
		}
		PartMID = Image_Part->GetDynamicMaterial();
	}
}

void UPDBodyPartImageWidget::OnValuesUpdated(float Current, float Max, float Percent)
{
	if (PartMID)
	{
		PartMID->SetScalarParameterValue(PercentParamName, Percent);
	}
}