// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/PDRootLayout.h"

#include "Widgets/PDWidgetStack.h"

UPDWidgetStack* UPDRootLayout::GetLayerStack(EUILayer Layer) const
{
	switch (Layer)
	{
	case EUILayer::Frontend: return Layer_Frontend;
	case EUILayer::GameMenu: return Layer_GameMenu;
	case EUILayer::Modal:    return Layer_Modal;
	}
	return nullptr;
}
