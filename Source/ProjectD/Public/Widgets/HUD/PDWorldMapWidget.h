#pragma once

#include "CoreMinimal.h"
#include "Widgets/PDActivatableBase.h"
#include "PDWorldMapWidget.generated.h"

class UCanvasPanel;
class UImage;
class UPDWorldMapDataAsset;
class UPDMapMarkerWidget;
class UPDFaintMarkWidget;
struct FPDMapMarker;
struct FPDFaintMark;

UCLASS(Abstract, Blueprintable)
class PROJECTD_API UPDWorldMapWidget : public UPDActivatableBase
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="WorldMap")
    TObjectPtr<UPDWorldMapDataAsset> WorldMapData;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="WorldMap")
    FVector2D MapWorldCenter = FVector2D::ZeroVector;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="WorldMap", meta=(ClampMin="100.0"))
    float MapWorldSize = 10000.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="WorldMap")
    float PlayerArrowAngleOffset = -90.f;

    //마커 위젯 클래스(BP에서 WBP_MapMarker 지정)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="WorldMap")
    TSubclassOf<UPDMapMarkerWidget> MapMarkerWidgetClass;
    
    //잔존 표식 위젯 클래스(BP에서 WBP_FaintMark 지정)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="WorldMap")
    TSubclassOf<UPDFaintMarkWidget> FaintMarkWidgetClass;
    
    //Zoom/Pan 설정
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="WorldMap|Zoom", meta=(ClampMin="0.1"))
    float ZoomMin = 0.5f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="WorldMap|Zoom", meta=(ClampMin="0.1"))
    float ZoomMax = 4.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="WorldMap|Zoom", meta=(ClampMin="1.01"))
    float ZoomStep = 1.15f;

    //현재 상태
    UPROPERTY(Transient, BlueprintReadOnly, Category="WorldMap|Zoom")
    float ZoomLevel = 1.0f;
    UPROPERTY(Transient, BlueprintReadOnly, Category="WorldMap|Zoom")
    FVector2D PanOffset = FVector2D::ZeroVector;
    
protected:
    UPROPERTY(meta=(BindWidget))
    TObjectPtr<UCanvasPanel> MapCanvas;

    UPROPERTY(meta=(BindWidget))
    TObjectPtr<UImage> MapBackground;

    UPROPERTY(meta=(BindWidget))
    TObjectPtr<UImage> PlayerArrow;

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& Geo, float DeltaTime) override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

    UFUNCTION()
    void HandleMarkerAdded(const FPDMapMarker& Marker);

    UFUNCTION()
    void HandleMarkerRemoved(int32 MarkerId);
    
    UFUNCTION()
    void HandleFaintMarkAdded(const FPDFaintMark& Mark);

    UFUNCTION()
    void HandleFaintMarkRemoved(int32 FaintId);
    
    virtual FReply NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
    // 월드 => 맵 캔버스 로컬 좌표
    FVector2D WorldToMap(const FVector& WorldPos) const;

    //맵 캔버스 로컬 좌표 (중앙 기준) => 월드(Z=0)
    FVector LocalToWorld(const FVector2D& LocalPos) const;

    //모든 마커 위젯에 DisplayIndex 재적용
    void RefreshAllMarkers();
    
    void SyncAllMarkerPositions();
    void SyncAllFaintMarkPositions();

    //활성 마커 위젯 추적
    UPROPERTY(Transient)
    TMap<int32, TObjectPtr<UPDMapMarkerWidget>> MarkerWidgets;
    
    //활성 표식 위젯 추적
    UPROPERTY(Transient)
    TMap<int32, TObjectPtr<UPDFaintMarkWidget>> FaintMarkWidgets;
    
    void SetZoomLevel(float NewLevel);
    void UpdateMapBackgroundTransform();

    //드래그 상태
    bool bIsPanning = false;
    FVector2D DragStartMousePos = FVector2D::ZeroVector;
    FVector2D DragStartPanOffset = FVector2D::ZeroVector;
};