#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "PDMapMarkerSubsystem.generated.h"

// 드맵 마커 데이터
USTRUCT(BlueprintType)
struct FPDMapMarker
{
    GENERATED_BODY()

    //고유 식별자. 자동 증가 (변경되지 않음)
    UPROPERTY(BlueprintReadOnly, Category="Map")
    int32 MarkerId = -1;

    //표시 번호 (1부터, 재정렬됨). 삭제 시 자동 재계산
    UPROPERTY(BlueprintReadOnly, Category="Map")
    int32 DisplayIndex = 0;

    //마커 월드 좌표
    UPROPERTY(BlueprintReadOnly, Category="Map")
    FVector WorldLocation = FVector::ZeroVector;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMapMarkerAdded, const FPDMapMarker&, Marker);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMapMarkerRemoved, int32, MarkerId);

UCLASS()
class PROJECTD_API UPDMapMarkerSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    //마커 추가. 새 MarkerId 반환 DisplayIndex 자동 부여
    UFUNCTION(BlueprintCallable, Category="Map")
    int32 AddMarker(const FVector& InWorldLocation);

    //MarkerId로 마커 제거 제거 후 DisplayIndex 재정렬
    UFUNCTION(BlueprintCallable, Category="Map")
    bool RemoveMarker(int32 InMarkerId);

    //해당 위치 근처 마커 제거(반경 내, 가장 가까운 마커 하나). fallback용
    UFUNCTION(BlueprintCallable, Category="Map")
    bool RemoveMarkerNearLocation(const FVector& InWorldLocation, float Radius = 300.f);

    //모든 마커 제거
    UFUNCTION(BlueprintCallable, Category="Map")
    void ClearAllMarkers();

    //활성 마커 목록(DisplayIndex순)
    UFUNCTION(BlueprintPure, Category="Map")
    void GetActiveMarkers(TArray<FPDMapMarker>& OutMarkers) const;

    UPROPERTY(BlueprintAssignable, Category="Map")
    FOnMapMarkerAdded OnMarkerAdded;

    UPROPERTY(BlueprintAssignable, Category="Map")
    FOnMapMarkerRemoved OnMarkerRemoved;

    virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;

private:
    //최대 마커 개수
    static constexpr int32 MaxActiveMarkers = 5;
    
    //모든 마커의 DisplayIndex를 MarkerId순으로 1부터 재계산
    void RecalculateDisplayIndices();

    UPROPERTY()
    TMap<int32, FPDMapMarker> ActiveMarkers;

    int32 NextMarkerId = 0;
};