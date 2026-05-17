#include "Widgets/HUD/PDCompassWidget.h"
#include "Ping/PDPingSubsystem.h"
#include "Ping/PDMapMarkerSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Components/Image.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"

void UPDCompassWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (CompassImage)
    {
        DynamicMat = CompassImage->GetDynamicMaterial();
    }

    UWorld* World = GetWorld();
    if (!World) return;

    //핑 Subsystem (잔존표식만 구독, 핑 인디케이터는 표시 안 함)
    if (UPDPingSubsystem* PingSub = World->GetSubsystem<UPDPingSubsystem>())
    {
        PingSub->OnFaintMarkAdded.AddDynamic(this, &UPDCompassWidget::HandleFaintMarkAdded);
        PingSub->OnFaintMarkRemoved.AddDynamic(this, &UPDCompassWidget::HandleFaintMarkRemoved);

        TArray<FPDFaintMark> ExistingFaints;
        PingSub->GetActiveFaintMarks(ExistingFaints);
        for (const FPDFaintMark& F : ExistingFaints) HandleFaintMarkAdded(F);
    }

    //마커 Subsystem
    if (UPDMapMarkerSubsystem* MarkerSub = World->GetSubsystem<UPDMapMarkerSubsystem>())
    {
        MarkerSub->OnMarkerAdded.AddDynamic(this, &UPDCompassWidget::HandleMarkerAdded);
        MarkerSub->OnMarkerRemoved.AddDynamic(this, &UPDCompassWidget::HandleMarkerRemoved);

        TArray<FPDMapMarker> Existing;
        MarkerSub->GetActiveMarkers(Existing);
        for (const FPDMapMarker& M : Existing) HandleMarkerAdded(M);
    }
}

void UPDCompassWidget::NativeDestruct()
{
    if (UWorld* World = GetWorld())
    {
        if (UPDPingSubsystem* PingSub = World->GetSubsystem<UPDPingSubsystem>())
        {
            PingSub->OnFaintMarkAdded.RemoveDynamic(this, &UPDCompassWidget::HandleFaintMarkAdded);
            PingSub->OnFaintMarkRemoved.RemoveDynamic(this, &UPDCompassWidget::HandleFaintMarkRemoved);
        }
        if (UPDMapMarkerSubsystem* MarkerSub = World->GetSubsystem<UPDMapMarkerSubsystem>())
        {
            MarkerSub->OnMarkerAdded.RemoveDynamic(this, &UPDCompassWidget::HandleMarkerAdded);
            MarkerSub->OnMarkerRemoved.RemoveDynamic(this, &UPDCompassWidget::HandleMarkerRemoved);
        }
    }

    for (auto& Pair : MarkerIndicators)      { if (Pair.Value) Pair.Value->RemoveFromParent(); }
    for (auto& Pair : FaintMarkIndicators)   { if (Pair.Value) Pair.Value->RemoveFromParent(); }
    MarkerIndicators.Empty();
    FaintMarkIndicators.Empty();

    Super::NativeDestruct();
}

void UPDCompassWidget::NativeTick(const FGeometry& Geo, float DeltaTime)
{
    Super::NativeTick(Geo, DeltaTime);

    if (!DynamicMat || !CompassImage) return;

    APlayerController* PC = GetOwningPlayer();
    if (!PC) return;

    float MouseX = 0.f, MouseY = 0.f;
    if (!PC->GetMousePosition(MouseX, MouseY)) return;

    int32 ViewportSizeX = 0, ViewportSizeY = 0;
    PC->GetViewportSize(ViewportSizeX, ViewportSizeY);
    if (ViewportSizeX <= 0 || ViewportSizeY <= 0) return;

    const FVector2D Center(ViewportSizeX * 0.5f, ViewportSizeY * 0.5f);
    const FVector2D Delta(MouseX - Center.X, MouseY - Center.Y);

    float Yaw = FMath::RadiansToDegrees(FMath::Atan2(Delta.X, -Delta.Y));
    while (Yaw < 0.f) Yaw += 360.f;
    while (Yaw >= 360.f) Yaw -= 360.f;

    const FVector2D ShowSize = CompassImage->GetCachedGeometry().GetLocalSize();
    const float Scale = (TextureFullWidth > 0.f) ? (ShowSize.X / TextureFullWidth) : 1.f;

    const float NormalizedYaw = Yaw / 360.f;
    const float UVOffset = NormalizedYaw - Scale * 0.5f;

    DynamicMat->SetScalarParameterValue(TEXT("UVOffset"), UVOffset);
    DynamicMat->SetScalarParameterValue(TEXT("UVScale"), Scale);

    SyncAllIndicators(Yaw, ShowSize, Scale);
}

void UPDCompassWidget::HandlePingAdded(const FPDPingData& Ping)
{
    if (!IndicatorCanvas) return;
    if (PingIndicators.Contains(Ping.PingId)) return;

    UTexture2D* IconTex = nullptr;
    if (TObjectPtr<UTexture2D>* Found = PingIconTextures.Find(Ping.PingType))
    {
        IconTex = *Found;
    }
    UImage* Indicator = CreateIndicatorImage(IconTex);
    if (!Indicator) return;

    PingIndicators.Add(Ping.PingId, Indicator);
}

void UPDCompassWidget::HandlePingRemoved(int32 PingId)
{
    TObjectPtr<UImage> Indicator;
    if (PingIndicators.RemoveAndCopyValue(PingId, Indicator))
    {
        if (Indicator) Indicator->RemoveFromParent();
    }
}

void UPDCompassWidget::HandleMarkerAdded(const FPDMapMarker& Marker)
{
    if (!IndicatorCanvas) return;
    if (MarkerIndicators.Contains(Marker.MarkerId)) return;

    UImage* Indicator = CreateIndicatorImage(MarkerIconTexture);
    if (!Indicator) return;

    MarkerIndicators.Add(Marker.MarkerId, Indicator);
}

void UPDCompassWidget::HandleMarkerRemoved(int32 MarkerId)
{
    TObjectPtr<UImage> Indicator;
    if (MarkerIndicators.RemoveAndCopyValue(MarkerId, Indicator))
    {
        if (Indicator) Indicator->RemoveFromParent();
    }
}

void UPDCompassWidget::HandleFaintMarkAdded(const FPDFaintMark& Mark)
{
    if (!IndicatorCanvas) return;
    if (FaintMarkIndicators.Contains(Mark.FaintId)) return;

    UImage* Indicator = CreateIndicatorImage(FaintMarkIconTexture);
    if (!Indicator) return;

    FaintMarkIndicators.Add(Mark.FaintId, Indicator);
}

void UPDCompassWidget::HandleFaintMarkRemoved(int32 FaintId)
{
    TObjectPtr<UImage> Indicator;
    if (FaintMarkIndicators.RemoveAndCopyValue(FaintId, Indicator))
    {
        if (Indicator) Indicator->RemoveFromParent();
    }
}

UImage* UPDCompassWidget::CreateIndicatorImage(UTexture2D* Texture)
{
    if (!IndicatorCanvas) return nullptr;

    UImage* Image = NewObject<UImage>(this);
    if (!Image) return nullptr;

    if (Texture)
    {
        Image->SetBrushFromTexture(Texture);
    }
    Image->SetDesiredSizeOverride(IndicatorSize);

    if (UCanvasPanelSlot* CPS = IndicatorCanvas->AddChildToCanvas(Image))
    {
        CPS->SetAnchors(FAnchors(0.5f, 0.5f));
        CPS->SetAlignment(FVector2D(0.5f, 1.0f)); //핀 아래쪽 끝 기준
        CPS->SetSize(IndicatorSize);
        CPS->SetPosition(FVector2D::ZeroVector);
    }
    return Image;
}

void UPDCompassWidget::UpdateIndicatorPosition(UImage* Indicator, const FVector& WorldPos, const FVector& PlayerLoc,
                                                float CurrentYaw, const FVector2D& ShowSize, float Scale)
{
    if (!Indicator) return;

    const FVector Delta = WorldPos - PlayerLoc;
    float TargetYaw = FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
    while (TargetYaw < 0.f) TargetYaw += 360.f;
    while (TargetYaw >= 360.f) TargetYaw -= 360.f;

    float Diff = TargetYaw - CurrentYaw;
    while (Diff < -180.f) Diff += 360.f;
    while (Diff > 180.f) Diff -= 360.f;

    const float VisibleAngle = Scale * 360.f;
    const float HalfVisible = VisibleAngle * 0.5f;
    const float CenterX = ShowSize.X * 0.5f;

    float IndicatorX;
    bool bOutOfView = false;

    if (FMath::Abs(Diff) <= HalfVisible && HalfVisible > 0.f)
    {
        IndicatorX = CenterX + (Diff / HalfVisible) * (ShowSize.X * 0.5f);
    }
    else
    {
        //시야 밖 => 좌우 끝
        IndicatorX = (Diff > 0) ? (ShowSize.X - 12.f) : 12.f;
        bOutOfView = true;
    }
    
    if (UCanvasPanelSlot* CPS = Cast<UCanvasPanelSlot>(Indicator->Slot))
    {
        CPS->SetPosition(FVector2D(IndicatorX - CenterX, -ShowSize.Y * 0.5f));//0.f => -ShowSize.Y * 0.5f
    }

    Indicator->SetOpacity(bOutOfView ? OutOfViewOpacity : 1.f);
}

void UPDCompassWidget::SyncAllIndicators(float CurrentYaw, const FVector2D& ShowSize, float Scale)
{
    APlayerController* PC = GetOwningPlayer();
    if (!PC) return;
    APawn* Pawn = PC->GetPawn();
    if (!Pawn) return;

    const FVector PlayerLoc = Pawn->GetActorLocation();

    UWorld* World = GetWorld();
    if (!World) return;

    //잔존표식만 (핑은 제외)
    if (UPDPingSubsystem* PingSub = World->GetSubsystem<UPDPingSubsystem>())
    {
        TArray<FPDFaintMark> Faints;
        PingSub->GetActiveFaintMarks(Faints);
        for (const FPDFaintMark& F : Faints)
        {
            if (TObjectPtr<UImage>* Found = FaintMarkIndicators.Find(F.FaintId))
            {
                if (*Found) UpdateIndicatorPosition(*Found, F.WorldLocation, PlayerLoc, CurrentYaw, ShowSize, Scale);
            }
        }
    }

    if (UPDMapMarkerSubsystem* MarkerSub = World->GetSubsystem<UPDMapMarkerSubsystem>())
    {
        TArray<FPDMapMarker> Markers;
        MarkerSub->GetActiveMarkers(Markers);
        for (const FPDMapMarker& M : Markers)
        {
            if (TObjectPtr<UImage>* Found = MarkerIndicators.Find(M.MarkerId))
            {
                if (*Found) UpdateIndicatorPosition(*Found, M.WorldLocation, PlayerLoc, CurrentYaw, ShowSize, Scale);
            }
        }
    }
}