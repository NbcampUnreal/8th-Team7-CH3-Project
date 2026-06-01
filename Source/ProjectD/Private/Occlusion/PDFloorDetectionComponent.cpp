#include "Occlusion/PDFloorDetectionComponent.h"
#include "Engine/World.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Occlusion/PDFloorOcclusionSubsystem.h"

UPDFloorDetectionComponent::UPDFloorDetectionComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UPDFloorDetectionComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UPDFloorDetectionComponent::OnEnteredBuildingFloor(FName BuildingGroupID, int32 FloorLevel)
{
    FBuildingFloorRef Ref;
    Ref.BuildingGroupID = BuildingGroupID;
    Ref.FloorLevel = FloorLevel;
    ActiveStack.Add(Ref);

    UpdateActiveFloor();
}

void UPDFloorDetectionComponent::OnLeftBuildingFloor(FName BuildingGroupID, int32 FloorLevel)
{
    //매칭되는 마지막 항목만 제거 (같은 박스 두 번 진입은 일반적으로 없지만 안전을 위해)
    for (int32 i = ActiveStack.Num() - 1; i >= 0; --i)
    {
        if (ActiveStack[i].BuildingGroupID == BuildingGroupID && ActiveStack[i].FloorLevel == FloorLevel)
        {
            ActiveStack.RemoveAt(i);
            break;
        }
    }

    UpdateActiveFloor();
}

void UPDFloorDetectionComponent::UpdateActiveFloor()
{
    FName NewBuilding = NAME_None;
    int32 NewFloor = INDEX_NONE;

    if (ActiveStack.Num() > 0)
    {
        const FBuildingFloorRef& Top = ActiveStack.Last();
        NewBuilding = Top.BuildingGroupID;
        NewFloor = Top.FloorLevel;
    }

    if (NewBuilding == CurrentBuildingGroupID && NewFloor == CurrentFloorLevel)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    UPDFloorOcclusionSubsystem* Subsystem = World->GetSubsystem<UPDFloorOcclusionSubsystem>();
    if (!Subsystem)
    {
        return;
    }

    // 빌딩이 바뀌면 이전 빌딩 가시성 풀기
    if (!CurrentBuildingGroupID.IsNone() && CurrentBuildingGroupID != NewBuilding)
    {
        Subsystem->ClearVisibleBuilding(CurrentBuildingGroupID);
    }

    CurrentBuildingGroupID = NewBuilding;
    CurrentFloorLevel = NewFloor;

    if (!NewBuilding.IsNone())
    {
        Subsystem->SetVisibleBuildingFloor(NewBuilding, NewFloor);
    }
    
    if (OcclusionMPC)
    {
        const float Strength = NewBuilding.IsNone() ? 1.0f : 0.0f;
        UKismetMaterialLibrary::SetScalarParameterValue(
            this, OcclusionMPC, TEXT("MaskStrength"), Strength);
    }
}