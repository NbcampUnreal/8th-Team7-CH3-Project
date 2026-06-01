// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/Crosshair/PDCrosshairWidget.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetLayoutLibrary.h"

void UPDCrosshairWidget::UpdateCrosshair(FVector2D MousePos, float Spread)
{
    // HUD의 Canvas Panel 자식으로 배치돼 있어야 동작. 부모가 Canvas가 아니면 위치 갱신은 무시되고 spread 콜백만 발화.
    if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot))
    {
        // GetMousePosition은 픽셀 좌표, CanvasPanelSlot은 DPI 적용된 위젯 좌표를 받으므로 ViewportScale로 보정.
        const float ViewportScale = UWidgetLayoutLibrary::GetViewportScale(this);
        const FVector2D LocalPos = (ViewportScale > KINDA_SMALL_NUMBER) ? (MousePos / ViewportScale) : MousePos;

        CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
        CanvasSlot->SetPosition(LocalPos);
    }

    if (!FMath::IsNearlyEqual(CurrentSpread, Spread))
    {
        CurrentSpread = Spread;
        OnSpreadChanged(CurrentSpread);
    }
}

void UPDCrosshairWidget::SetCrosshairType(EWeaponType NewType)
{
    if (CurrentType == NewType) return;
    CurrentType = NewType;
    OnCrosshairTypeChanged(CurrentType);
}