#pragma once

#include "CoreMinimal.h"
#include "Widgets/PDActivatableBase.h"
#include "Ping/PDPingTypes.h"
#include "PDCompassWidget.generated.h"

class UImage;
class UCanvasPanel;
class UTexture2D;
class UMaterialInstanceDynamic;
struct FPDPingData;
struct FPDMapMarker;
struct FPDFaintMark;

UCLASS(Abstract, Blueprintable)
class PROJECTD_API UPDCompassWidget : public UPDActivatableBase
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Compass")
    float TextureFullWidth = 1440.f;

    //핑 종류별 아이콘 텍스처
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Compass|Indicators")
    TMap<EPDPingType, TObjectPtr<UTexture2D>> PingIconTextures;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Compass|Indicators")
    TObjectPtr<UTexture2D> MarkerIconTexture;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Compass|Indicators")
    TObjectPtr<UTexture2D> FaintMarkIconTexture;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Compass|Indicators")
    FVector2D IndicatorSize = FVector2D(28.f, 28.f);

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Compass|Indicators", meta=(ClampMin="0.0", ClampMax="1.0"))
    float OutOfViewOpacity = 0.5f;

protected:
    UPROPERTY(meta=(BindWidget))
    TObjectPtr<UImage> CompassImage;

    UPROPERTY(meta=(BindWidget))
    TObjectPtr<UCanvasPanel> IndicatorCanvas;

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& Geo, float DeltaTime) override;

    UFUNCTION()
    void HandlePingAdded(const FPDPingData& Ping);

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
    TObjectPtr<UMaterialInstanceDynamic> DynamicMat;

    UPROPERTY(Transient)
    TMap<int32, TObjectPtr<UImage>> PingIndicators;

    UPROPERTY(Transient)
    TMap<int32, TObjectPtr<UImage>> MarkerIndicators;

    UPROPERTY(Transient)
    TMap<int32, TObjectPtr<UImage>> FaintMarkIndicators;

    UImage* CreateIndicatorImage(UTexture2D* Texture);

    void UpdateIndicatorPosition(UImage* Indicator, const FVector& WorldPos, const FVector& PlayerLoc,
                                  float CurrentYaw, const FVector2D& ShowSize, float Scale);

    void SyncAllIndicators(float CurrentYaw, const FVector2D& ShowSize, float Scale);
};