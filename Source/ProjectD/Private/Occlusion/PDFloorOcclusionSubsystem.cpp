#include "Occlusion/PDFloorOcclusionSubsystem.h"
#include "Engine/World.h"
#include "Occlusion/PDFloorOcclusionComponent.h"

void UPDFloorOcclusionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void UPDFloorOcclusionSubsystem::Deinitialize()
{
    ComponentsByBuildingFloor.Empty();
    VisibleFloorByBuilding.Empty();
    ActiveFades.Empty();

    Super::Deinitialize();
}

bool UPDFloorOcclusionSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
    return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

TStatId UPDFloorOcclusionSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UPDFloorOcclusionSubsystem, STATGROUP_Tickables);
}

void UPDFloorOcclusionSubsystem::Tick(float DeltaTime)
{
    if (ActiveFades.Num() == 0)
    {
        return;
    }

    const float Step = FadeSpeed * DeltaTime;

    for (int32 i = ActiveFades.Num() - 1; i >= 0; --i)
    {
        FFadeTarget& Fade = ActiveFades[i];
        UPDFloorOcclusionComponent* Comp = Fade.Component.Get();
        if (!Comp)
        {
            ActiveFades.RemoveAt(i);
            continue;
        }

        const float Current = Comp->GetFadeAmount();
        const float Next = FMath::FInterpConstantTo(Current, Fade.TargetAlpha, 1.0f, Step);
        Comp->SetFadeAmount(Next);

        if (FMath::IsNearlyEqual(Next, Fade.TargetAlpha, 0.001f))
        {
            Comp->SetFadeAmount(Fade.TargetAlpha);
            ActiveFades.RemoveAt(i);
        }
    }
}

void UPDFloorOcclusionSubsystem::RegisterFloorActor(UPDFloorOcclusionComponent* Component)
{
    if (!Component)
    {
        return;
    }

    const FName GroupID = Component->GetBuildingGroupID();
    const int32 Floor = Component->GetFloorLevel();

    TMap<int32, TArray<TWeakObjectPtr<UPDFloorOcclusionComponent>>>& FloorMap = ComponentsByBuildingFloor.FindOrAdd(GroupID);
    TArray<TWeakObjectPtr<UPDFloorOcclusionComponent>>& Bucket = FloorMap.FindOrAdd(Floor);
    Bucket.AddUnique(Component);

    //신규 등록은 즉시 적용 (보간 없이)
    const float Target = CalculateTargetAlpha(GroupID, Floor);
    Component->SetFadeAmount(Target);
}

void UPDFloorOcclusionSubsystem::UnregisterFloorActor(UPDFloorOcclusionComponent* Component)
{
    if (!Component)
    {
        return;
    }

    const FName GroupID = Component->GetBuildingGroupID();
    const int32 Floor = Component->GetFloorLevel();

    if (TMap<int32, TArray<TWeakObjectPtr<UPDFloorOcclusionComponent>>>* FloorMap = ComponentsByBuildingFloor.Find(GroupID))
    {
        if (TArray<TWeakObjectPtr<UPDFloorOcclusionComponent>>* Bucket = FloorMap->Find(Floor))
        {
            Bucket->RemoveSingle(Component);
        }
    }

    ActiveFades.RemoveAll([Component](const FFadeTarget& F) { return F.Component.Get() == Component; });
}

void UPDFloorOcclusionSubsystem::SetVisibleBuildingFloor(FName BuildingGroupID, int32 VisibleFloor)
{
    int32* Existing = VisibleFloorByBuilding.Find(BuildingGroupID);
    if (Existing && *Existing == VisibleFloor)
    {
        return;
    }

    VisibleFloorByBuilding.Add(BuildingGroupID, VisibleFloor);
    RefreshBuildingFades(BuildingGroupID);
}

void UPDFloorOcclusionSubsystem::ClearVisibleBuilding(FName BuildingGroupID)
{
    int32* Existing = VisibleFloorByBuilding.Find(BuildingGroupID);
    if (Existing && *Existing == INDEX_NONE)
    {
        return;
    }

    VisibleFloorByBuilding.Add(BuildingGroupID, INDEX_NONE);
    RefreshBuildingFades(BuildingGroupID);
}

void UPDFloorOcclusionSubsystem::RequestFade(UPDFloorOcclusionComponent* Component, float TargetAlpha)
{
    if (!Component)
    {
        return;
    }

    if (FMath::IsNearlyEqual(Component->GetFadeAmount(), TargetAlpha, 0.001f))
    {
        return;
    }

    for (FFadeTarget& Existing : ActiveFades)
    {
        if (Existing.Component.Get() == Component)
        {
            Existing.TargetAlpha = TargetAlpha;
            return;
        }
    }

    FFadeTarget NewFade;
    NewFade.Component = Component;
    NewFade.TargetAlpha = TargetAlpha;
    ActiveFades.Add(NewFade);
}

float UPDFloorOcclusionSubsystem::CalculateTargetAlpha(FName BuildingGroupID, int32 FloorLevel) const
{
    const int32* VisibleFloor = VisibleFloorByBuilding.Find(BuildingGroupID);

    //빌딩에 진입한 적 없거나 외부 => 모두 보임
    if (!VisibleFloor || *VisibleFloor == INDEX_NONE)
    {
        return 1.0f;
    }

    //활성층보다 높은 층만 가림
    return (FloorLevel > *VisibleFloor) ? 0.0f : 1.0f;
}

void UPDFloorOcclusionSubsystem::RefreshBuildingFades(FName BuildingGroupID)
{
    TMap<int32, TArray<TWeakObjectPtr<UPDFloorOcclusionComponent>>>* FloorMap = ComponentsByBuildingFloor.Find(BuildingGroupID);
    if (!FloorMap)
    {
        return;
    }

    for (auto& Pair : *FloorMap)
    {
        const float Target = CalculateTargetAlpha(BuildingGroupID, Pair.Key);
        for (TWeakObjectPtr<UPDFloorOcclusionComponent>& Weak : Pair.Value)
        {
            if (UPDFloorOcclusionComponent* Comp = Weak.Get())
            {
                RequestFade(Comp, Target);
            }
        }
    }
}