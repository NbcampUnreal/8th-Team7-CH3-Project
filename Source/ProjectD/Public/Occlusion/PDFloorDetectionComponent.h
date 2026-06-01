#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PDFloorDetectionComponent.generated.h"

class UMaterialParameterCollection;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTD_API UPDFloorDetectionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UPDFloorDetectionComponent();

    void OnEnteredBuildingFloor(FName BuildingGroupID, int32 FloorLevel);
    void OnLeftBuildingFloor(FName BuildingGroupID, int32 FloorLevel);

protected:
    virtual void BeginPlay() override;
    
    //MPC_PDOcclusion참조 - MaskStrength 갱신용
    UPROPERTY(EditAnywhere, Category = "Floor")
    TObjectPtr<UMaterialParameterCollection> OcclusionMPC;
    
private:
    //중첩 영역 지원 : 같은 빌딩 안에서 두 박스 겹칠 수 있음 (계단 경계 등)
    struct FBuildingFloorRef
    {
        FName BuildingGroupID;
        int32 FloorLevel;
    };
    TArray<FBuildingFloorRef> ActiveStack;

    FName CurrentBuildingGroupID;
    int32 CurrentFloorLevel = INDEX_NONE;

    void UpdateActiveFloor();
};