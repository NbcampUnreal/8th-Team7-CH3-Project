#include "Widgets/HUD/PDWorldMapWidget.h"
#include "Data/PDWorldMapDataAsset.h"
#include "Widgets/HUD/PDMapMarkerWidget.h"
#include "Ping/PDFaintMarkWidget.h"
#include "Ping/PDMapMarkerSubsystem.h"
#include "Ping/PDPingSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Engine/World.h"

void UPDWorldMapWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (WorldMapData)
    {
        const FPDWorldMapEntry Entry = WorldMapData->GetEntryForWorld(GetWorld());
        MapWorldCenter = Entry.WorldCenter;
        MapWorldSize = Entry.WorldSize;
        PlayerArrowAngleOffset = Entry.PlayerArrowAngleOffset;

        if (MapBackground && Entry.MapTexture)
        {
            MapBackground->SetBrushFromTexture(Entry.MapTexture);
        }
    }

    UWorld* World = GetWorld();
    if (!World) return;

    //마커 Subsystem 구독
    if (UPDMapMarkerSubsystem* Sub = World->GetSubsystem<UPDMapMarkerSubsystem>())
    {
        Sub->OnMarkerAdded.AddDynamic(this, &UPDWorldMapWidget::HandleMarkerAdded);
        Sub->OnMarkerRemoved.AddDynamic(this, &UPDWorldMapWidget::HandleMarkerRemoved);

        //위젯 열린 시점에 이미 있는 마커들 그리기
        TArray<FPDMapMarker> Existing;
        Sub->GetActiveMarkers(Existing);
        for (const FPDMapMarker& M : Existing)
        {
            HandleMarkerAdded(M);
        }
    }

    //Ping Subsystem 잔존 표식 구독
    if (UPDPingSubsystem* PingSub = World->GetSubsystem<UPDPingSubsystem>())
    {
        PingSub->OnFaintMarkAdded.AddDynamic(this, &UPDWorldMapWidget::HandleFaintMarkAdded);
        PingSub->OnFaintMarkRemoved.AddDynamic(this, &UPDWorldMapWidget::HandleFaintMarkRemoved);

        //위젯 열린 시점에 이미 있는 잔존 표식들 그리기
        TArray<FPDFaintMark> ExistingF;
        PingSub->GetActiveFaintMarks(ExistingF);
        for (const FPDFaintMark& F : ExistingF)
        {
            HandleFaintMarkAdded(F);
        }
    }
    
    //Zoom/Pan초기화
    ZoomLevel = 1.0f;
    PanOffset = FVector2D::ZeroVector;
    bIsPanning = false;
    UpdateMapBackgroundTransform();
}

void UPDWorldMapWidget::NativeDestruct()
{
    if (UWorld* World = GetWorld())
    {
        if (UPDMapMarkerSubsystem* Sub = World->GetSubsystem<UPDMapMarkerSubsystem>())
        {
            Sub->OnMarkerAdded.RemoveDynamic(this, &UPDWorldMapWidget::HandleMarkerAdded);
            Sub->OnMarkerRemoved.RemoveDynamic(this, &UPDWorldMapWidget::HandleMarkerRemoved);
        }
        if (UPDPingSubsystem* PingSub = World->GetSubsystem<UPDPingSubsystem>())
        {
            PingSub->OnFaintMarkAdded.RemoveDynamic(this, &UPDWorldMapWidget::HandleFaintMarkAdded);
            PingSub->OnFaintMarkRemoved.RemoveDynamic(this, &UPDWorldMapWidget::HandleFaintMarkRemoved);
        }
    }

    for (auto& Pair : MarkerWidgets)
    {
        if (Pair.Value) Pair.Value->RemoveFromParent();
    }
    MarkerWidgets.Empty();

    for (auto& Pair : FaintMarkWidgets)
    {
        if (Pair.Value) Pair.Value->RemoveFromParent();
    }
    FaintMarkWidgets.Empty();

    Super::NativeDestruct();
}

void UPDWorldMapWidget::NativeTick(const FGeometry& Geo, float DeltaTime)
{
    Super::NativeTick(Geo, DeltaTime);

    APlayerController* PC = GetOwningPlayer();
    if (!PC) return;
    APawn* Pawn = PC->GetPawn();
    if (!Pawn || !PlayerArrow) return;

    const FVector PlayerLoc = Pawn->GetActorLocation();
    const float PlayerYaw = Pawn->GetActorRotation().Yaw;

    if (UCanvasPanelSlot* PlayerSlot = Cast<UCanvasPanelSlot>(PlayerArrow->Slot))
    {
        PlayerSlot->SetPosition(WorldToMap(PlayerLoc));
    }
    PlayerArrow->SetRenderTransformAngle(PlayerYaw + PlayerArrowAngleOffset);
    
    SyncAllMarkerPositions();
    SyncAllFaintMarkPositions();
    UpdateMapBackgroundTransform(); 
}

FReply UPDWorldMapWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{   
    //드래그 시작
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && MapCanvas)
    {
        bIsPanning = true;
        DragStartMousePos = InMouseEvent.GetScreenSpacePosition();
        DragStartPanOffset = PanOffset;
        return FReply::Handled().CaptureMouse(TakeWidget());
    }

    
    if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton && MapCanvas)
    {
        //클릭 화면 좌표 => MapCanvas 로컬 좌표 => 중앙 기준 상대 좌표 => 월드 좌표
        const FVector2D ScreenPos = InMouseEvent.GetScreenSpacePosition();
        const FGeometry CanvasGeo = MapCanvas->GetCachedGeometry();
        const FVector2D CanvasLocal = CanvasGeo.AbsoluteToLocal(ScreenPos);
        const FVector2D CanvasSize = CanvasGeo.GetLocalSize();
  
        //캔버스 영역 밖 우클릭은 무시(어두운 배경 클릭 등)
        if (CanvasLocal.X < 0.f || CanvasLocal.X > CanvasSize.X ||
            CanvasLocal.Y < 0.f || CanvasLocal.Y > CanvasSize.Y)
        {
            return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
        }
        
        const FVector2D Centered(CanvasLocal.X - CanvasSize.X * 0.5f, CanvasLocal.Y - CanvasSize.Y * 0.5f);
        const FVector WorldPos = LocalToWorld(Centered);

        if (UWorld* World = GetWorld())
        {
            if (UPDMapMarkerSubsystem* Sub = World->GetSubsystem<UPDMapMarkerSubsystem>())
            {
                Sub->AddMarker(WorldPos);
            }
        }
        return FReply::Handled();
    }

    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UPDWorldMapWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (bIsPanning)
    {
        const FVector2D Current = InMouseEvent.GetScreenSpacePosition();
        PanOffset = DragStartPanOffset + (Current - DragStartMousePos);
        return FReply::Handled();
    }
    return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

FReply UPDWorldMapWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bIsPanning)
    {
        bIsPanning = false;
        return FReply::Handled().ReleaseMouseCapture();
    }
    return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

FReply UPDWorldMapWidget::NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    const float Delta = InMouseEvent.GetWheelDelta();
    if (FMath::IsNearlyZero(Delta))
    {
        return Super::NativeOnMouseWheel(InGeometry, InMouseEvent);
    }

    const float Factor = Delta > 0 ? ZoomStep : 1.f / ZoomStep;
    SetZoomLevel(ZoomLevel * Factor);
    return FReply::Handled();
}

void UPDWorldMapWidget::HandleMarkerAdded(const FPDMapMarker& Marker)
{
    if (!MapCanvas || !MapMarkerWidgetClass) return;
    if (MarkerWidgets.Contains(Marker.MarkerId)) return;

    UPDMapMarkerWidget* Widget = CreateWidget<UPDMapMarkerWidget>(this, MapMarkerWidgetClass);
    if (!Widget) return;

    Widget->MarkerId = Marker.MarkerId;
    Widget->WorldLocation = Marker.WorldLocation;
    Widget->SetDisplayIndex(Marker.DisplayIndex);

    if (UCanvasPanelSlot* PanelSlot = MapCanvas->AddChildToCanvas(Widget))
    {
        PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f));
        PanelSlot->SetAlignment(FVector2D(0.5f, 0.833f));
        PanelSlot->SetSize(FVector2D(32.f, 45.f));
        PanelSlot->SetPosition(WorldToMap(Marker.WorldLocation));
    }

    MarkerWidgets.Add(Marker.MarkerId, Widget);
}

void UPDWorldMapWidget::HandleMarkerRemoved(int32 MarkerId)
{
    TObjectPtr<UPDMapMarkerWidget> Widget;
    if (MarkerWidgets.RemoveAndCopyValue(MarkerId, Widget))
    {
        if (Widget) Widget->RemoveFromParent();
    }

    //남은 마커들의 DisplayIndex 재적용
    RefreshAllMarkers();
}

void UPDWorldMapWidget::HandleFaintMarkAdded(const FPDFaintMark& Mark)
{
    if (!MapCanvas || !FaintMarkWidgetClass) return;
    if (FaintMarkWidgets.Contains(Mark.FaintId)) return;

    UPDFaintMarkWidget* Widget = CreateWidget<UPDFaintMarkWidget>(this, FaintMarkWidgetClass);
    if (!Widget) return;

    Widget->FaintId = Mark.FaintId;
    Widget->WorldLocation = Mark.WorldLocation;

    if (UCanvasPanelSlot* PanelSlot = MapCanvas->AddChildToCanvas(Widget))
    {
        PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f));
        PanelSlot->SetAlignment(FVector2D(0.5f, 0.833f));
        PanelSlot->SetSize(FVector2D(32.f, 42.f));
        PanelSlot->SetPosition(WorldToMap(Mark.WorldLocation));
    }

    FaintMarkWidgets.Add(Mark.FaintId, Widget);
}

void UPDWorldMapWidget::HandleFaintMarkRemoved(int32 FaintId)
{
    TObjectPtr<UPDFaintMarkWidget> Widget;
    if (FaintMarkWidgets.RemoveAndCopyValue(FaintId, Widget))
    {
        if (Widget) Widget->RemoveFromParent();
    }
}

void UPDWorldMapWidget::RefreshAllMarkers()
{
    UWorld* World = GetWorld();
    if (!World) return;
    UPDMapMarkerSubsystem* Sub = World->GetSubsystem<UPDMapMarkerSubsystem>();
    if (!Sub) return;

    TArray<FPDMapMarker> Markers;
    Sub->GetActiveMarkers(Markers);

    for (const FPDMapMarker& M : Markers)
    {
        if (TObjectPtr<UPDMapMarkerWidget>* WidgetPtr = MarkerWidgets.Find(M.MarkerId))
        {
            if (*WidgetPtr)
            {
                (*WidgetPtr)->SetDisplayIndex(M.DisplayIndex);
            }
        }
    }
}

FVector2D UPDWorldMapWidget::WorldToMap(const FVector& WorldPos) const
{
    if (!MapCanvas || MapWorldSize <= 0.f) return FVector2D::ZeroVector;

    const FVector2D Delta(WorldPos.X - MapWorldCenter.X, WorldPos.Y - MapWorldCenter.Y);
    const FVector2D CanvasSize = MapCanvas->GetCachedGeometry().GetLocalSize();
    const float HalfX = CanvasSize.X * 0.5f;
    const float HalfY = CanvasSize.Y * 0.5f;
    const float HalfWorld = MapWorldSize * 0.5f;

    const float ScreenX = (Delta.Y / HalfWorld) * HalfX * ZoomLevel + PanOffset.X;
    const float ScreenY = -(Delta.X / HalfWorld) * HalfY * ZoomLevel + PanOffset.Y;

    return FVector2D(ScreenX, ScreenY);
}

FVector UPDWorldMapWidget::LocalToWorld(const FVector2D& LocalPos) const
{
    if (!MapCanvas || MapWorldSize <= 0.f || ZoomLevel <= 0.f) return FVector::ZeroVector;

    const FVector2D CanvasSize = MapCanvas->GetCachedGeometry().GetLocalSize();
    const float HalfX = CanvasSize.X * 0.5f;
    const float HalfY = CanvasSize.Y * 0.5f;
    const float HalfWorld = MapWorldSize * 0.5f;

    //줌/팬 역적용
    const float AdjustedX = (LocalPos.X - PanOffset.X) / ZoomLevel;
    const float AdjustedY = (LocalPos.Y - PanOffset.Y) / ZoomLevel;

    const float WorldY = (AdjustedX / HalfX) * HalfWorld;
    const float WorldX = -(AdjustedY / HalfY) * HalfWorld;

    return FVector(MapWorldCenter.X + WorldX, MapWorldCenter.Y + WorldY, 0.f);
}

void UPDWorldMapWidget::SyncAllMarkerPositions()
{
    if (!MapCanvas) return;
    //캔버스 레이아웃 전에는 스킵 => 다음 틱에 자동 복구
    const FVector2D CanvasSize = MapCanvas->GetCachedGeometry().GetLocalSize();
    if (CanvasSize.X <= 0.f || CanvasSize.Y <= 0.f) return;

    UWorld* World = GetWorld();
    if (!World) return;

    UPDMapMarkerSubsystem* Sub = World->GetSubsystem<UPDMapMarkerSubsystem>();
    if (!Sub) return;

    TArray<FPDMapMarker> Markers;
    Sub->GetActiveMarkers(Markers);

    for (const FPDMapMarker& M : Markers)
    {
        TObjectPtr<UPDMapMarkerWidget>* Found = MarkerWidgets.Find(M.MarkerId);
        if (!Found || !*Found) continue;

        if (UCanvasPanelSlot* CPS = Cast<UCanvasPanelSlot>((*Found)->Slot))
        {
            CPS->SetPosition(WorldToMap(M.WorldLocation));
        }
    }
}

void UPDWorldMapWidget::SyncAllFaintMarkPositions()
{
    if (!MapCanvas) return;
    const FVector2D CanvasSize = MapCanvas->GetCachedGeometry().GetLocalSize();
    if (CanvasSize.X <= 0.f || CanvasSize.Y <= 0.f) return;

    UWorld* World = GetWorld();
    if (!World) return;

    UPDPingSubsystem* Sub = World->GetSubsystem<UPDPingSubsystem>();
    if (!Sub) return;

    TArray<FPDFaintMark> Faints;
    Sub->GetActiveFaintMarks(Faints);

    for (const FPDFaintMark& F : Faints)
    {
        TObjectPtr<UPDFaintMarkWidget>* Found = FaintMarkWidgets.Find(F.FaintId);
        if (!Found || !*Found) continue;

        if (UCanvasPanelSlot* CPS = Cast<UCanvasPanelSlot>((*Found)->Slot))
        {
            CPS->SetPosition(WorldToMap(F.WorldLocation));
        }
    }
}

void UPDWorldMapWidget::SetZoomLevel(float NewLevel)
{
    NewLevel = FMath::Clamp(NewLevel, ZoomMin, ZoomMax);
    if (FMath::IsNearlyEqual(NewLevel, ZoomLevel)) return;

    //화면 중앙 기준 줌: 보던 위치가 화면 중앙에 유지되도록 PanOffset 비례 조정
    const float Ratio = NewLevel / ZoomLevel;
    PanOffset *= Ratio;
    ZoomLevel = NewLevel;
}

void UPDWorldMapWidget::UpdateMapBackgroundTransform()
{
    if (!MapBackground) return;
    MapBackground->SetRenderScale(FVector2D(ZoomLevel, ZoomLevel));
    MapBackground->SetRenderTranslation(PanOffset);
}

