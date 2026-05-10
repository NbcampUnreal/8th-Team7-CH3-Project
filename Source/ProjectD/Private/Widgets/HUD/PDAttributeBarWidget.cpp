// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/HUD/PDAttributeBarWidget.h"

void UPDAttributeBarWidget::SetValues(float Current, float Max)
{
	const float Percent = Max > 0.f ? FMath::Clamp(Current / Max, 0.f, 1.f) : 0.f;
	OnValuesUpdated(Current, Max, Percent);
}
