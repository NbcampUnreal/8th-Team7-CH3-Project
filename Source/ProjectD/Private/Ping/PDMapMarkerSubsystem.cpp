#include "Ping/PDMapMarkerSubsystem.h"

bool UPDMapMarkerSubsystem::DoesSupportWorldType(EWorldType::Type WorldType) const
{
    return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

int32 UPDMapMarkerSubsystem::AddMarker(const FVector& InWorldLocation)
{
    if (ActiveMarkers.Num() >= MaxActiveMarkers)
    {
        int32 OldestId = INT_MAX;
        for (const TPair<int32, FPDMapMarker>& Pair : ActiveMarkers)
        {
            if (Pair.Key < OldestId)
            {
                OldestId = Pair.Key;
            }
        }
        if (OldestId != INT_MAX)
        {
            RemoveMarker(OldestId);
        }
    }
    
    FPDMapMarker NewMarker;
    NewMarker.MarkerId = NextMarkerId++;
    NewMarker.WorldLocation = InWorldLocation;

    ActiveMarkers.Add(NewMarker.MarkerId, NewMarker);
    RecalculateDisplayIndices();

    //재계산 후 갱신된 데이터
    const FPDMapMarker& Added = ActiveMarkers[NewMarker.MarkerId];
    OnMarkerAdded.Broadcast(Added);
    return NewMarker.MarkerId;
}

bool UPDMapMarkerSubsystem::RemoveMarker(int32 InMarkerId)
{
    if (ActiveMarkers.Remove(InMarkerId) > 0)
    {
        RecalculateDisplayIndices();
        OnMarkerRemoved.Broadcast(InMarkerId);
        return true;
    }
    return false;
}

bool UPDMapMarkerSubsystem::RemoveMarkerNearLocation(const FVector& InWorldLocation, float Radius)
{
    int32 BestId = -1;
    float BestDistSq = Radius * Radius;

    for (const TPair<int32, FPDMapMarker>& Pair : ActiveMarkers)
    {
        const float DistSq = FVector::DistSquared2D(Pair.Value.WorldLocation, InWorldLocation);
        if (DistSq < BestDistSq)
        {
            BestDistSq = DistSq;
            BestId = Pair.Key;
        }
    }

    if (BestId != -1)
    {
        return RemoveMarker(BestId);
    }
    return false;
}

void UPDMapMarkerSubsystem::ClearAllMarkers()
{
    TArray<int32> Ids;
    ActiveMarkers.GetKeys(Ids);
    for (int32 Id : Ids)
    {
        RemoveMarker(Id);
    }
}

void UPDMapMarkerSubsystem::GetActiveMarkers(TArray<FPDMapMarker>& OutMarkers) const
{
    OutMarkers.Reset();
    ActiveMarkers.GenerateValueArray(OutMarkers);

    //DisplayIndex순 정렬
    OutMarkers.Sort([](const FPDMapMarker& A, const FPDMapMarker& B)
    {
        return A.DisplayIndex < B.DisplayIndex;
    });
}

void UPDMapMarkerSubsystem::RecalculateDisplayIndices()
{
    //MarkerId순으로 정렬해서 1부터 번호 매김(찍힌 순서대로)
    TArray<int32> SortedIds;
    ActiveMarkers.GetKeys(SortedIds);
    SortedIds.Sort();

    for (int32 i = 0; i < SortedIds.Num(); ++i)
    {
        if (FPDMapMarker* Marker = ActiveMarkers.Find(SortedIds[i]))
        {
            Marker->DisplayIndex = i + 1;
        }
    }
}