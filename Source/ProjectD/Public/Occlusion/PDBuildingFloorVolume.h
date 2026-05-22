#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PDBuildingFloorVolume.generated.h"

class UBoxComponent;

UCLASS()
class PROJECTD_API APDBuildingFloorVolume : public AActor
{
    GENERATED_BODY()

public:
    APDBuildingFloorVolume();

    UFUNCTION(BlueprintPure, Category = "Floor")
    int32 GetFloorLevel() const { return FloorLevel; }

    UFUNCTION(BlueprintPure, Category = "Floor")
    FName GetBuildingGroupID() const { return BuildingGroupID; }

protected:
    virtual void BeginPlay() override;

    UFUNCTION()
    void HandleBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void HandleEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

    //박스 영역 = 이 층의 실제 영역. 외벽 안쪽까지 정확히 그려야 함
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Floor")
    TObjectPtr<UBoxComponent> TriggerBox;

    //0 = 1층
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Floor")
    int32 FloorLevel = 0;

    //같은 빌딩의 박스들끼리 묶는 ID. 예: "Building_22"
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Floor")
    FName BuildingGroupID;

private:
    //박스 영역 내 모든 StaticMeshActor에 UPDFloorOcclusionComponent 자동 부착
    void AttachComponentsToOverlappingMeshes();
    void CheckInitialPawnOverlap();
};