#pragma once

#include "CoreMinimal.h"
#include "Widgets/PDActivatableBase.h"
#include "Ping/PDPingTypes.h"
#include "PDMinimapWidget.generated.h"

class UCanvasPanel;
class UPDPingIconWidget;
class UPDMapMarkerWidget;
class UPDFaintMarkWidget;
class UTexture2D;
struct FPDMapMarker;
struct FPDFaintMark;

UCLASS(Abstract, Blueprintable)
class PROJECTD_API UPDMinimapWidget : public UPDActivatableBase
{
    GENERATED_BODY()

public:
    //미니맵 표시 반경 (월드 단위, cm)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Minimap")
    float MapRadius = 2000.f;

    //핑 아이콘 위젯 클래스
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Minimap")
    TSubclassOf<UPDPingIconWidget> PingIconClass;

    //핑 타입별 텍스처
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Minimap")
    TMap<EPDPingType, TObjectPtr<UTexture2D>> IconTextures;

    //미니맵이 플레이어 회전 따라가는지
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Minimap")
    bool bRotateWithPlayer = false;

    //미니맵용 마커 아이콘 클래스 (WBP_MapMarker 재사용)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Minimap")
    TSubclassOf<UPDMapMarkerWidget> MapMarkerIconClass;

    //미니맵용 잔존 표식 위젯 클래스 (WBP_FaintMark 재사용)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Minimap")
    TSubclassOf<UPDFaintMarkWidget> FaintMarkWidgetClass;

protected:
    UPROPERTY(meta=(BindWidget))
    TObjectPtr<UCanvasPanel> PingCanvas;

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& Geo, float DeltaTime) override;

    //BP에서 플레이어 위치/회전 활용
    UFUNCTION(BlueprintImplementableEvent, Category="Minimap")
    void OnPlayerTransformUpdated(const FVector& Location, float Yaw);

    UFUNCTION()
    void HandlePingAdded(const FPDPingData& PingData);

    UFUNCTION()
    void HandlePingRemoved(int32 PingId);

    UFUNCTION()
    void HandleMarkerAdded(const FPDMapMarker& Marker);

    UFUNCTION()
    void HandleMarkerRemoved(int32 MarkerId);

    UFUNCTION()
    void HandleFaintMarkAdded(const FPDFaintMark& Mark);

    UFUNCTION()
    void HandleFaintMarkRemoved(int32 FaintId);

private:
    UPROPERTY(Transient)
    TMap<int32, TObjectPtr<UPDPingIconWidget>> PingIcons;

    UPROPERTY(Transient)
    TMap<int32, TObjectPtr<UPDMapMarkerWidget>> MapMarkerWidgets;

    UPROPERTY(Transient)
    TMap<int32, TObjectPtr<UPDFaintMarkWidget>> FaintMarkWidgets;

    //남은 마커들 DisplayIndex 재적용
    void RefreshMapMarkerIndices();

    //월드 좌표 => 미니맵 캔버스 로컬 좌표 변환
    FVector2D WorldToMinimap(const FVector& WorldPos) const;
};