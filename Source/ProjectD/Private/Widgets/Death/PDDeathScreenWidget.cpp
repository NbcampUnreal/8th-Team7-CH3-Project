// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/Death/PDDeathScreenWidget.h"

UPDDeathScreenWidget::UPDDeathScreenWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InputMode = EWidgetInputMode::Menu;
	bShowMouseCursor = false;
}