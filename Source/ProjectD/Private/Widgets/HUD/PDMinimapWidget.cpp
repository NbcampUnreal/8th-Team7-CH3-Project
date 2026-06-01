#include "Widgets/HUD/PDMinimapWidget.h"

#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Ping/PDPingSubsystem.h"
#include "Ping/PDMapMarkerSubsystem.h"
#include "Ping/PDPingIconWidget.h"
#include "Widgets/HUD/PDMapMarkerWidget.h"
#include "Ping/PDFaintMarkWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"

void UPDMinimapWidget::NativeConstruct()
{
    Super::NativeConstruct();

    UWorld* World = GetWorld();
    if (!World) return;

    //Ping Subsystem 구독(핑 + 잔존 표식)
    if (UPDPingSubsystem* PingSys = World->GetSubsystem<UPDPingSubsystem>())
    {
        PingSys->OnPingAdded.AddDynamic(this, &UPDMinimapWidget::HandlePingAdded);
        PingSys->OnPingRemoved.AddDynamic(this, &UPDMinimapWidget::HandlePingRemoved);

        //위젯 생성 시점에 이미 떠있는 핑들 그리기
        TArray<FPDPingData> Existing;
        PingSys->GetActivePings(Existing);
        for (const FPDPingData& P : Existing)
        {
            HandlePingAdded(P);
        }

        PingSys->OnFaintMarkAdded.AddDynamic(this, &UPDMinimapWidget::HandleFaintMarkAdded);
        PingSys->OnFaintMarkRemoved.AddDynamic(this, &UPDMinimapWidget::HandleFaintMarkRemoved);

        //위젯 생성 시점에 이미 떠있는 잔존 표식 그리기
        TArray<FPDFaintMark> ExistingF;
        PingSys->GetActiveFaintMarks(ExistingF);
        for (const FPDFaintMark& F : ExistingF)
        {
            HandleFaintMarkAdded(F);
        }
    }

    //MapMarker Subsystem 구독
    if (UPDMapMarkerSubsystem* MarkerSub = World->GetSubsystem<UPDMapMarkerSubsystem>())
    {
        MarkerSub->OnMarkerAdded.AddDynamic(this, &UPDMinimapWidget::HandleMarkerAdded);
        MarkerSub->OnMarkerRemoved.AddDynamic(this, &UPDMinimapWidget::HandleMarkerRemoved);

        TArray<FPDMapMarker> ExistingM;
        MarkerSub->GetActiveMarkers(ExistingM);
        for (const FPDMapMarker& M : ExistingM)
        {
            HandleMarkerAdded(M);
        }
    }
}

void UPDMinimapWidget::NativeDestruct()
{
    if (UWorld* World = GetWorld())
    {
        if (UPDPingSubsystem* PingSys = World->GetSubsystem<UPDPingSubsystem>())
        {
            PingSys->OnPingAdded.RemoveDynamic(this, &UPDMinimapWidget::HandlePingAdded);
            PingSys->OnPingRemoved.RemoveDynamic(this, &UPDMinimapWidget::HandlePingRemoved);
            PingSys->OnFaintMarkAdded.RemoveDynamic(this, &UPDMinimapWidget::HandleFaintMarkAdded);
            PingSys->OnFaintMarkRemoved.RemoveDynamic(this, &UPDMinimapWidget::HandleFaintMarkRemoved);
        }
        if (UPDMapMarkerSubsystem* MarkerSub = World->GetSubsystem<UPDMapMarkerSubsystem>())
        {
            MarkerSub->OnMarkerAdded.RemoveDynamic(this, &UPDMinimapWidget::HandleMarkerAdded);
            MarkerSub->OnMarkerRemoved.RemoveDynamic(this, &UPDMinimapWidget::HandleMarkerRemoved);
        }
    }
    
    //위젯 정리
    for (auto& Pair : PingIcons)
    {
        if (Pair.Value) Pair.Value->RemoveFromParent();
    }
    PingIcons.Empty();

    for (auto& Pair : MapMarkerWidgets)
    {
        if (Pair.Value) Pair.Value->RemoveFromParent();
    }
    MapMarkerWidgets.Empty();

    for (auto& Pair : FaintMarkWidgets)
    {
        if (Pair.Value) Pair.Value->RemoveFromParent();
    }
    FaintMarkWidgets.Empty();

    Super::NativeDestruct();
}

void UPDMinimapWidget::NativeTick(const FGeometry& Geo, float DeltaTime)
{
    Super::NativeTick(Geo, DeltaTime);

    APlayerController* PC = GetOwningPlayer();
    if (!PC) return;
    APawn* Pawn = PC->GetPawn();
    if (!Pawn) return;

    const FVector PlayerLoc = Pawn->GetActorLocation();
    const float PlayerYaw = Pawn->GetActorRotation().Yaw;
    OnPlayerTransformUpdated(PlayerLoc, PlayerYaw);

    if (!PingCanvas) return;

    //핑 아이콘 위치 갱신
    for (const auto& Pair : PingIcons)
    {
        UPDPingIconWidget* Icon = Pair.Value;
        if (!Icon) continue;
        if (UCanvasPanelSlot* SlotPtr = Cast<UCanvasPanelSlot>(Icon->Slot))
        {
            SlotPtr->SetPosition(WorldToMinimap(Icon->WorldLocation));
        }
    }

    //마커 위치 갱신
    for (const auto& Pair : MapMarkerWidgets)
    {
        UPDMapMarkerWidget* Marker = Pair.Value;
        if (!Marker) continue;
        if (UCanvasPanelSlot* SlotPtr = Cast<UCanvasPanelSlot>(Marker->Slot))
        {
            SlotPtr->SetPosition(WorldToMinimap(Marker->WorldLocation));
        }
    }

    //잔존 표식 위치 갱신
    for (const auto& Pair : FaintMarkWidgets)
    {
        UPDFaintMarkWidget* Faint = Pair.Value;
        if (!Faint) continue;
        if (UCanvasPanelSlot* SlotPtr = Cast<UCanvasPanelSlot>(Faint->Slot))
        {
            SlotPtr->SetPosition(WorldToMinimap(Faint->WorldLocation));
        }
    }
}

void UPDMinimapWidget::HandlePingAdded(const FPDPingData& PingData)
{
    if (!PingCanvas || !PingIconClass) return;
    if (PingIcons.Contains(PingData.PingId)) return;

    UPDPingIconWidget* Icon = CreateWidget<UPDPingIconWidget>(this, PingIconClass);
    if (!Icon) return;

    Icon->PingId = PingData.PingId;
    Icon->WorldLocation = PingData.WorldLocation;

    if (TObjectPtr<UTexture2D>* TexPtr = IconTextures.Find(PingData.PingType))
    {
        if (UTexture2D* Tex = *TexPtr)
        {
            Icon->SetIconTexture(Tex);
        }
    }

    if (UCanvasPanelSlot* PanelSlot = PingCanvas->AddChildToCanvas(Icon))
    {
        PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f));
        PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
        PanelSlot->SetSize(FVector2D(24.f, 24.f));
        PanelSlot->SetPosition(WorldToMinimap(PingData.WorldLocation));
    }

    PingIcons.Add(PingData.PingId, Icon);
}

void UPDMinimapWidget::HandlePingRemoved(int32 PingId)
{
    TObjectPtr<UPDPingIconWidget> Icon;
    if (PingIcons.RemoveAndCopyValue(PingId, Icon))
    {
        if (Icon) Icon->RemoveFromParent();
    }
}

void UPDMinimapWidget::HandleMarkerAdded(const FPDMapMarker& Marker)
{
    if (!PingCanvas || !MapMarkerIconClass) return;
    if (MapMarkerWidgets.Contains(Marker.MarkerId)) return;

    UPDMapMarkerWidget* Widget = CreateWidget<UPDMapMarkerWidget>(this, MapMarkerIconClass);
    if (!Widget) return;

    Widget->MarkerId = Marker.MarkerId;
    Widget->WorldLocation = Marker.WorldLocation;
    Widget->SetDisplayIndex(Marker.DisplayIndex);

    if (UCanvasPanelSlot* PanelSlot = PingCanvas->AddChildToCanvas(Widget))
    {
        PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f));
        PanelSlot->SetAlignment(FVector2D(0.5f, 0.833f));
        PanelSlot->SetSize(FVector2D(20.f, 28.f));
        PanelSlot->SetPosition(WorldToMinimap(Marker.WorldLocation));
    }

    MapMarkerWidgets.Add(Marker.MarkerId, Widget);
}

void UPDMinimapWidget::HandleMarkerRemoved(int32 MarkerId)
{
    TObjectPtr<UPDMapMarkerWidget> Widget;
    if (MapMarkerWidgets.RemoveAndCopyValue(MarkerId, Widget))
    {
        if (Widget) Widget->RemoveFromParent();
    }
    RefreshMapMarkerIndices();
}

void UPDMinimapWidget::HandleFaintMarkAdded(const FPDFaintMark& Mark)
{
    if (!PingCanvas || !FaintMarkWidgetClass) return;
    if (FaintMarkWidgets.Contains(Mark.FaintId)) return;

    UPDFaintMarkWidget* Widget = CreateWidget<UPDFaintMarkWidget>(this, FaintMarkWidgetClass);
    if (!Widget) return;

    Widget->FaintId = Mark.FaintId;
    Widget->WorldLocation = Mark.WorldLocation;

    if (UCanvasPanelSlot* PanelSlot = PingCanvas->AddChildToCanvas(Widget))
    {
        PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f));
        PanelSlot->SetAlignment(FVector2D(0.5f, 0.833f));
        PanelSlot->SetSize(FVector2D(20.f, 26.f));
        PanelSlot->SetPosition(WorldToMinimap(Mark.WorldLocation));
    }

    FaintMarkWidgets.Add(Mark.FaintId, Widget);
}

void UPDMinimapWidget::HandleFaintMarkRemoved(int32 FaintId)
{
    TObjectPtr<UPDFaintMarkWidget> Widget;
    if (FaintMarkWidgets.RemoveAndCopyValue(FaintId, Widget))
    {
        if (Widget) Widget->RemoveFromParent();
    }
}

void UPDMinimapWidget::RefreshMapMarkerIndices()
{
    UWorld* World = GetWorld();
    if (!World) return;
    UPDMapMarkerSubsystem* Sub = World->GetSubsystem<UPDMapMarkerSubsystem>();
    if (!Sub) return;

    TArray<FPDMapMarker> Markers;
    Sub->GetActiveMarkers(Markers);

    for (const FPDMapMarker& M : Markers)
    {
        if (TObjectPtr<UPDMapMarkerWidget>* WidgetPtr = MapMarkerWidgets.Find(M.MarkerId))
        {
            if (*WidgetPtr)
            {
                (*WidgetPtr)->SetDisplayIndex(M.DisplayIndex);
            }
        }
    }
}

FVector2D UPDMinimapWidget::WorldToMinimap(const FVector& WorldPos) const
{
    APlayerController* PC = GetOwningPlayer();
    if (!PC || !PingCanvas) return FVector2D::ZeroVector;
    APawn* Pawn = PC->GetPawn();
    if (!Pawn) return FVector2D::ZeroVector;

    const FVector PlayerLoc = Pawn->GetActorLocation();
    FVector Delta = WorldPos - PlayerLoc;

    if (bRotateWithPlayer)
    {
        const float Yaw = Pawn->GetActorRotation().Yaw;
        Delta = FRotator(0.f, -Yaw, 0.f).RotateVector(Delta);
    }

    const FVector2D CanvasSize = PingCanvas->GetCachedGeometry().GetLocalSize();
    const float HalfX = CanvasSize.X * 0.5f;
    const float HalfY = CanvasSize.Y * 0.5f;

    const float ScreenX = (Delta.Y / MapRadius) * HalfX;
    const float ScreenY = -(Delta.X / MapRadius) * HalfY;

    return FVector2D(ScreenX, ScreenY);
}